/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>

#define MODULE mesh_module
#include "modules_common.h"
#include "mesh_module_event.h"
#include "uart_module_event.h"
#include "robot_module_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_MESH_MODULE_LOG_LEVEL);

struct mesh_msg_data
{
	union {
		struct uart_module_event uart;
		struct robot_module_event robot;
	} module;
};

/* Mesh module message queue. */
#define MESH_QUEUE_ENTRY_COUNT		10
#define MESH_QUEUE_BYTE_ALIGNMENT	4

K_MSGQ_DEFINE(msgq_mesh, sizeof(struct mesh_msg_data),
	      MESH_QUEUE_ENTRY_COUNT, MESH_QUEUE_BYTE_ALIGNMENT);

/* mesh module super states. */
static enum state_type {
	STATE_MESH_NOT_READY,
	STATE_MESH_READY,
} state;

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type state)
{
	switch (state)
	{
	case STATE_MESH_NOT_READY:
		return "STATE_MESH_NOT_READY";
	case STATE_MESH_READY:
		return "STATE_MESH_READY";
	default:
		return "Unknown state";
	}
}

static void state_set(enum state_type new_state)
{
	if (new_state == state)
	{
		LOG_DBG("State: %s", state2str(state));
		return;
	}

	LOG_DBG("State transition %s --> %s",
			state2str(state),
			state2str(new_state));

	state = new_state;
}

/* Event handlers */
static bool app_event_handler(const struct app_event_header *aeh)
{
	struct mesh_msg_data msg = {0};
	bool enqueue_msg = false;

	if (is_robot_module_event(aeh))
	{
		struct robot_module_event *evt = cast_robot_module_event(aeh);
		msg.module.robot = *evt;
		enqueue_msg = true;
	}

	if (is_uart_module_event(aeh))
	{
		struct uart_module_event *evt = cast_uart_module_event(aeh);
		msg.module.uart = *evt;
		enqueue_msg = true;
	}

	if (enqueue_msg)
	{
		int err = k_msgq_put(&msgq_mesh, &msg, K_NO_WAIT);

		if (err)
		{
			LOG_ERR("Message could not be enqueued");
		}
	}

	return false;
}

/* Module state handlers. */
static void on_state_mesh_ready(struct mesh_msg_data *msg)
{
	
}

static void on_all_states(struct mesh_msg_data *msg)
{
	if (IS_EVENT(msg, uart, UART_EVT_RX)) {
		struct mesh_module_event *event = new_mesh_module_event();

		switch (msg->module.uart.data.rx.header.id)
		{
		case ID_CLI_MODEL_ID:
			if(msg->module.uart.data.rx.header.type == BT_MESH_ID_OP_STATUS)
			{
				event->type = MESH_EVT_ROBOT_ID;
				struct bt_mesh_id_status id;
				memcpy((void*)&id.id, msg->module.uart.data.rx.data, 6);
				event->data.robot_id = id;
				event->addr = msg->module.uart.data.rx.header.addr;
				
				LOG_INF("addr: %x", msg->module.uart.data.rx.header.addr);
			} else {
				LOG_WRN("Received message is not supported, %d", msg->module.uart.data.rx.header.type);
			}
			break;
		case MOVEMENT_CLI_MODEL_ID:
			if(msg->module.uart.data.rx.header.type == BT_MESH_MOVEMENT_OP_MOVEMENT_ACK)
			{
				event->type = MESH_EVT_MOVEMENT_CONFIGURED;
				event->addr = msg->module.uart.data.rx.header.addr;
				
				LOG_INF("addr: %x", msg->module.uart.data.rx.header.addr);
			} else {
				LOG_WRN("Received message is not supported, %d", msg->module.uart.data.rx.header.type);
			}
			break;
		
		default:
			break;
		}
		
		APP_EVENT_SUBMIT(event);
	}

	if (IS_EVENT(msg, robot, ROBOT_EVT_MOVEMENT_CONFIGURE)) {
		
		struct uart_module_event *event = new_uart_module_event();
		event->type = UART_EVT_TX;
		event->data.tx.header.len = 9;
		event->data.tx.header.type = BT_MESH_MOVEMENT_OP_MOVEMENT_SET;
		event->data.tx.header.id = MOVEMENT_CLI_MODEL_ID;
		event->data.tx.header.addr = msg->module.robot.addr; 
		event->data.tx.data = msg->module.robot.data.movement;
		LOG_INF("event->data.tx.data %p", event->data.tx.data);
		LOG_HEXDUMP_INF(event->data.tx.data, 9, "payload:");
		APP_EVENT_SUBMIT(event);
	}

	if (IS_EVENT(msg, robot, ROBOT_EVT_CLEAR_TO_MOVE)) {
		
		struct uart_module_event *event = new_uart_module_event();
		event->type = UART_EVT_TX;
		event->data.tx.header.len = 0;
		event->data.tx.header.type = BT_MESH_MOVEMENT_OP_READY_SET;
		event->data.tx.header.id = MOVEMENT_CLI_MODEL_ID;
		event->data.tx.header.addr = 0xFFFF; 
		event->data.tx.data = NULL;
		// LOG_INF("event->data.tx.data %p", event->data.tx.data);
		// LOG_HEXDUMP_INF(event->data.tx.data, 1, "payload:");
		APP_EVENT_SUBMIT(event);
	}
}

static void module_thread_fn(void)
{
	LOG_INF("Mesh module thread started");
	state_set(STATE_MESH_NOT_READY);


	LOG_DBG("MESH initialized");

	struct mesh_msg_data msg = {0};
	while (true)
	{
		k_msgq_get(&msgq_mesh, &msg, K_FOREVER);

		switch (state)
		{
		case STATE_MESH_READY:
		{
			on_state_mesh_ready(&msg);
			break;
		}
		default:
			break;
		}

		on_all_states(&msg);
	}
}

K_THREAD_DEFINE(mesh_module_thread, CONFIG_MESH_THREAD_STACK_SIZE,
				module_thread_fn, NULL, NULL, NULL,
				K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, robot_module_event);
APP_EVENT_SUBSCRIBE(MODULE, uart_module_event);