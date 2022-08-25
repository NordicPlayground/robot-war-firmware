/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/slist.h>
#include <stdio.h>

#define MODULE robot_module

#include "robot_module_event.h"
#include "cloud_module_event.h"
#include "mesh_module_event.h"
#include "ui_module_event.h"

#include <zephyr/logging/log.h>
#define ROBOT_MODULE_LOG_LEVEL 4
LOG_MODULE_REGISTER(MODULE, CONFIG_ROBOT_MODULE_LOG_LEVEL);

/* Robot module super states. */
static enum state_type {
	STATE_CLOUD_DISCONNECTED,
	STATE_CLOUD_CONNECTED,
} state;

enum robot_state {
	ROBOT_STATE_READY,
	ROBOT_STATE_CONFIGURING,
	ROBOT_STATE_CONFIGURED,
};

struct robot {
	sys_snode_t node;
	char id[13];
	uint16_t addr;
	enum robot_state state;
	struct codec_movement movement;
	uint8_t revolutions;
	struct codec_led led;
};

static sys_slist_t robot_list;

struct robot_msg_data {
	union {
		struct ui_module_event ui;
		struct robot_module_event robot;
		struct cloud_module_event cloud;
		struct mesh_module_event mesh;
	} event;
};

typedef void (robot_fn)(struct robot *robot);
typedef void (robot_delta_fn)(struct robot *robot, const char *delta, size_t len);


/* Robot module message queue. */
#define ROBOT_QUEUE_ENTRY_COUNT		10
#define ROBOT_QUEUE_BYTE_ALIGNMENT	4

K_MSGQ_DEFINE(msgq_robot, sizeof(struct robot_msg_data),
	      ROBOT_QUEUE_ENTRY_COUNT, ROBOT_QUEUE_BYTE_ALIGNMENT);

int version_prev = 0;

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type state)
{
	switch (state) {
	case STATE_CLOUD_DISCONNECTED:
		return "STATE_CLOUD_DISCONNECTED";
	case STATE_CLOUD_CONNECTED:
		return "STATE_CLOUD_CONNECTED";
	default:
		return "Unknown state";
	}
}

static void state_set(enum state_type new_state)
{
	if (new_state == state) {
		LOG_DBG("State: %s", state2str(state));
		return;
	}

	LOG_DBG("State transition %s --> %s",
		state2str(state),
		state2str(new_state));

	state = new_state;
}

/* Handlers */
static bool app_event_handler(const struct app_event_header *aeh)
{
	struct robot_msg_data msg = {0};
	bool enqueue_msg = false;

	if (is_robot_module_event(aeh)) {
		struct robot_module_event *evt = cast_robot_module_event(aeh);
		msg.event.robot = *evt;
		enqueue_msg = true;
	}

	if (is_cloud_module_event(aeh)) {
		struct cloud_module_event *evt = cast_cloud_module_event(aeh);
		msg.event.cloud = *evt;
		enqueue_msg = true;
	}

	if (is_ui_module_event(aeh)) {
		struct ui_module_event *evt = cast_ui_module_event(aeh);

		msg.event.ui = *evt;
		enqueue_msg = true;
	}

	if (is_mesh_module_event(aeh)) {
		struct mesh_module_event *evt = cast_mesh_module_event(aeh);

		msg.event.mesh = *evt;
		enqueue_msg = true;
	}

	if (enqueue_msg)
	{
		int err = k_msgq_put(&msgq_robot, &msg, K_NO_WAIT);

		if (err)
		{
			LOG_ERR("Message could not be enqueued");
		}
	}

	return false;
}

/* Internal robot functions */
static void set_led_event(struct robot *robot)
{
	struct robot_module_event *event = new_robot_module_event();
	event->type = ROBOT_EVT_LED_CONFIGURE;
	event->addr = robot->addr;
	event->data.led = &robot->led;
	APP_EVENT_SUBMIT(event);
}

static struct robot * add_robot(uint64_t id, uint16_t addr) 
{
	struct robot *robot;

	robot = k_malloc(sizeof(struct robot));
	sprintf(robot->id, "%x", (uint32_t) ((id >> 16) & 0xffffffff));
	sprintf(&robot->id[4], "%x", (uint32_t) (id & 0xffffffff));
	
	robot->addr = addr;
	robot->movement.drive_time = 0;
	robot->movement.rotation = 0;
	robot->movement.speed = 100;
	robot->revolutions = 0;
	robot->state = ROBOT_STATE_READY;

	sys_slist_append(&robot_list, &robot->node);
	return robot;
}

static struct robot* get_robot_by_addr(uint16_t addr)
{
	struct robot *robot;
	SYS_SLIST_FOR_EACH_CONTAINER(&robot_list, robot, node) {
		if (robot->addr == addr) {
				return robot;
		}
	}
	return NULL;
}

static void for_each_robot(robot_fn *func)
{
	struct robot *robot;
	SYS_SLIST_FOR_EACH_CONTAINER(&robot_list, robot, node) {
		if (func) {
			func(robot);
		}
	}
}

static void report_event(char * report)
{
	struct robot_module_event *event = new_robot_module_event();
	event->type = ROBOT_EVT_REPORT;
	event->data.str = report;
	APP_EVENT_SUBMIT(event);
}

static void report_clear_robot_list(void) 
{
	report_event(codec_encode_remove_robots_report());
}

static void report_robot_movement(struct robot *robot) 
{
	char *report = codec_encode_movement_report(robot->id, robot->movement);
	report_event(report);
	// k_free(report);
}

static void report_robot_revolution_count(struct robot *robot) 
{
	// LOG_INF("revolution count: %d", robot->revolutions);
	report_event(codec_encode_revolution_count_report(robot->id, robot->revolutions));
}

static void report_robot(struct robot *robot) 
{	
	report_robot_movement(robot);
	report_robot_revolution_count(robot);

}

static bool all_robots_are_in_state(enum robot_state state) 
{
	struct robot *robot;
	SYS_SLIST_FOR_EACH_CONTAINER(&robot_list, robot, node) {
		LOG_INF("addr: %d, state: %d", robot->addr, robot->state);
		if (robot->state != state) {
			return false;
		}
	}
	return true;
}

static void process_delta_for_each_robot(robot_delta_fn *func, const char *delta, size_t len)
{
	struct robot *robot;
	SYS_SLIST_FOR_EACH_CONTAINER(&robot_list, robot, node) {
		if (func) {
			func(robot, delta, len);
		}
	}
}

static void process_delta_led(struct robot *robot, const char *delta, size_t len) 
{
	// struct codec_led led;
	struct robot_module_event *event;

	if(codec_decode_led(robot->id, delta, len, &robot->led)) {
		// robot->led = led;
		event = new_robot_module_event();
		event->type = ROBOT_EVT_LED_CONFIGURE;
		event->addr = robot->addr;
		event->data.led = &robot->led;
		APP_EVENT_SUBMIT(event);
	}
}

static void process_delta_movement(struct robot *robot, const char *delta, size_t len) 
{
	// struct codec_movement movement;
	struct robot_module_event *event;

	if(codec_decode_movement(robot->id, delta, len, &robot->movement)) {
		// robot->movement = movement;
		robot->movement.speed = 100;
		LOG_INF("robot->movement.drive_time: %d", robot->movement.drive_time);

		event = new_robot_module_event();
		event->type = ROBOT_EVT_MOVEMENT_CONFIGURE;
		event->addr = robot->addr;
		event->data.movement = &robot->movement;
		APP_EVENT_SUBMIT(event);
	}
}

/* Message handler for STATE_CONFIGURING. */
static void on_state_cloud_disconnected(struct robot_msg_data *msg)
{
	if (is_mesh_module_event((struct app_event_header *)(&msg->event.mesh)))
    {
        if (msg->event.mesh.type == MESH_EVT_ROBOT_ID)
        {
			// LOG_INF("Robot id detected addr: %x, id %llx", msg->event.mesh.addr, msg->event.mesh.data.robot_id.id);
			if(!get_robot_by_addr(msg->event.mesh.addr)) {
				add_robot(msg->event.mesh.data.robot_id.id, msg->event.mesh.addr);
				struct robot *robot = get_robot_by_addr(msg->event.mesh.addr);
				robot->led.red = 0;
				robot->led.green = 230;
				robot->led.blue = 10;
				robot->led.time = 500;

				set_led_event(robot);
			}
			
		}
	}

	if (is_cloud_module_event((struct app_event_header *)(&msg->event.cloud)))
    {
        if (msg->event.cloud.type == CLOUD_EVT_CONNECTED)
        {
			report_clear_robot_list();
			// report_event(codec_encode_remove_robots_report());
			for_each_robot(report_robot);

			state_set(STATE_CLOUD_CONNECTED);
		}
	}
}

/* Message handler for STATE_EXECUTING. */
static void on_state_cloud_connected(struct robot_msg_data *msg)
{
	if (is_cloud_module_event((struct app_event_header *)(&msg->event.cloud)))
    {
        if (msg->event.cloud.type == CLOUD_EVT_UPDATE_DELTA)
        {
			int version;

			const char * delta = msg->event.cloud.data.pub_msg.ptr;
			size_t len = msg->event.cloud.data.pub_msg.len;

			version = codec_decode_version(delta, len);
			if (version_prev >= version) {
				return;
			}
			version_prev = version;
			process_delta_for_each_robot(process_delta_movement, delta, len);
			process_delta_for_each_robot(process_delta_led, delta, len);
		}
	}

	if (is_cloud_module_event((struct app_event_header *)(&msg->event.cloud)))
    {
        if (msg->event.cloud.type == CLOUD_EVT_DISCONNECTED)
        {
			state_set(STATE_CLOUD_DISCONNECTED);
		}
	}

	if (is_mesh_module_event((struct app_event_header *)(&msg->event.mesh)))
    {
        if (msg->event.mesh.type == MESH_EVT_ROBOT_ID)
        {
			if(!get_robot_by_addr(msg->event.mesh.addr)) {
				struct robot *robot = add_robot(msg->event.mesh.data.robot_id.id, msg->event.mesh.addr);
				report_robot(robot);
				robot->led.red = 0;
				robot->led.green = 230;
				robot->led.blue = 10;
				robot->led.time = 500;

				set_led_event(robot);
			}
		}
	}

	if (is_mesh_module_event((struct app_event_header *)(&msg->event.mesh)))
    {
        if (msg->event.mesh.type == MESH_EVT_MOVEMENT_CONFIGURED)
        {
			struct robot *robot = get_robot_by_addr(msg->event.mesh.addr);
			// if (robot->state != ROBOT_STATE_READY) {
			// 	return;
			// }
			report_robot_movement(robot);
			robot->state = ROBOT_STATE_CONFIGURED;
			robot->led.red = 230;
			robot->led.green = 0;
			robot->led.blue = 10;
			robot->led.time = 150;

			set_led_event(robot);

			if (all_robots_are_in_state(ROBOT_STATE_CONFIGURED)) {
				struct robot_module_event *clear_to_move_event = new_robot_module_event();
				clear_to_move_event->type = ROBOT_EVT_CLEAR_TO_MOVE;
				APP_EVENT_SUBMIT(clear_to_move_event);
			}
		}
	}

	if (is_mesh_module_event((struct app_event_header *)(&msg->event.mesh)))
    {
        if (msg->event.mesh.type == MESH_EVT_TELEMETRY_REPORTED)
        {
			LOG_INF("telemetry reported from addr %d", msg->event.mesh.addr);
			struct robot *robot = get_robot_by_addr(msg->event.mesh.addr);
			robot->revolutions = msg->event.mesh.data.report.revolutions;
			robot->state = ROBOT_STATE_READY;

			robot->led.red = 0;
			robot->led.green = 230;
			robot->led.blue = 10;
			robot->led.time = 500;

			set_led_event(robot);

			if (all_robots_are_in_state(ROBOT_STATE_READY)) {
				for_each_robot(report_robot_revolution_count);
			}
		}
	}
}

static void module_thread_fn(void)
{
	struct robot_msg_data msg;

	LOG_INF("Robot module thread started");

	sys_slist_init(&robot_list);

	while (true) {
		k_msgq_get(&msgq_robot, &msg, K_FOREVER);

		switch (state) {
		case STATE_CLOUD_DISCONNECTED:
			on_state_cloud_disconnected(&msg);
			break;
		case STATE_CLOUD_CONNECTED:
			on_state_cloud_connected(&msg);
			break;
		default:
			break;
		}
	}
}

K_THREAD_DEFINE(robot_module_thread, CONFIG_ROBOT_THREAD_STACK_SIZE,
		module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, robot_module_event);
APP_EVENT_SUBSCRIBE(MODULE, cloud_module_event);
APP_EVENT_SUBSCRIBE(MODULE, ui_module_event);
APP_EVENT_SUBSCRIBE(MODULE, mesh_module_event);