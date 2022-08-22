/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <net/aws_iot.h>
#include <nrf_modem_at.h>
#include <string.h>
#include <qos.h>
#include <net/socket.h>

#define MODULE cloud_module

#include "modules_common.h"
#include "modem_module_event.h"
#include "robot_module_event.h"
#include "cloud_module_event.h"

#include <zephyr/logging/log.h>
#define CLOUD_MODULE_LOG_LEVEL 4
LOG_MODULE_REGISTER(MODULE, CONFIG_CLOUD_MODULE_LOG_LEVEL);

#define TOPIC_UPDATE_DELTA "$aws/things/" CONFIG_AWS_IOT_CLIENT_ID_STATIC "/shadow/update/delta"
#define TOPIC_GET_ACCEPTED "$aws/things/" CONFIG_AWS_IOT_CLIENT_ID_STATIC "/shadow/get/accepted"
#define TOPIC_UPDATE_ACCEPTED "$aws/things/" CONFIG_AWS_IOT_CLIENT_ID_STATIC "/shadow/update/accepted"
#define TOPIC_UPDATE_REJECTED "$aws/things/" CONFIG_AWS_IOT_CLIENT_ID_STATIC "/shadow/update/rejected"

QOS_MESSAGE_TYPES_REGISTER(CLOUD_SHADOW_UPDATE);
QOS_MESSAGE_TYPES_REGISTER(CLOUD_SHADOW_CLEAR);

struct aws_iot_config aws_config;

K_SEM_DEFINE(cloud_connected_sem, 0, 1);
struct cloud_msg_data {
	union {
		struct cloud_module_event cloud;
		struct modem_module_event modem;
		struct robot_module_event robot;
	} event;
};

/* Cloud module super states. */
static enum state_type {
	STATE_LTE_DISCONNECTED,
	STATE_LTE_CONNECTED,
} state;

/* Cloud module sub states. */
static enum sub_state_type {
	SUB_STATE_CLOUD_DISCONNECTED,
	SUB_STATE_CLOUD_CONNECTED,
} sub_state;

static struct k_work_delayable connect_check_work;

struct cloud_backoff_delay_lookup {
	int delay;
};

/* Lookup table for backoff reconnection to cloud. Binary scaling. */
static struct cloud_backoff_delay_lookup backoff_delay[] = {
	{ 32 }, { 64 }, { 128 }, { 256 }, { 512 },
	{ 2048 }, { 4096 }, { 8192 }, { 16384 }, { 32768 },
	{ 65536 }, { 131072 }, { 262144 }, { 524288 }, { 1048576 }
};

/* Variable that keeps track of how many times a reconnection to cloud
 * has been tried without success.
 */
static int connect_retries;

/* Cloud module message queue. */
#define CLOUD_QUEUE_ENTRY_COUNT		20
#define CLOUD_QUEUE_BYTE_ALIGNMENT	4

K_MSGQ_DEFINE(msgq_cloud, sizeof(struct cloud_msg_data),
	      CLOUD_QUEUE_ENTRY_COUNT, CLOUD_QUEUE_BYTE_ALIGNMENT);

/* Forward declarations. */
static void connect_check_work_fn(struct k_work *work);

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type state)
{
	switch (state) {
	case STATE_LTE_DISCONNECTED:
		return "STATE_LTE_DISCONNECTED";
	case STATE_LTE_CONNECTED:
		return "STATE_LTE_CONNECTED";
	default:
		return "Unknown";
	}
}

static char *sub_state2str(enum sub_state_type new_state)
{
	switch (new_state) {
	case SUB_STATE_CLOUD_DISCONNECTED:
		return "SUB_STATE_CLOUD_DISCONNECTED";
	case SUB_STATE_CLOUD_CONNECTED:
		return "SUB_STATE_CLOUD_CONNECTED";
	default:
		return "Unknown";
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

static void sub_state_set(enum sub_state_type new_state)
{
	if (new_state == sub_state) {
		LOG_DBG("Sub state: %s", sub_state2str(sub_state));
		return;
	}

	LOG_DBG("Sub state transition %s --> %s",
		sub_state2str(sub_state),
		sub_state2str(new_state));

	sub_state = new_state;
}

static inline int is_topic(const struct aws_iot_evt *evt, char* topic) 
{
	return (0 == strcmp(evt->data.msg.topic.str, topic));
}

/* Handlers */
static bool app_event_handler(const struct app_event_header *aeh)
{
	struct cloud_msg_data msg = {0};
	bool enqueue_msg = false;

	if (is_cloud_module_event(aeh)) {
		struct cloud_module_event *evt = cast_cloud_module_event(aeh);
		msg.event.cloud = *evt;
		enqueue_msg = true;
	}

	if (is_modem_module_event(aeh)) {
		struct modem_module_event *evt = cast_modem_module_event(aeh);
		msg.event.modem = *evt;
		enqueue_msg = true;
	}

	if (is_robot_module_event(aeh)) {
		struct robot_module_event *evt = cast_robot_module_event(aeh);

		msg.event.robot = *evt;
		enqueue_msg = true;
	}

	if (enqueue_msg)
	{
		int err = k_msgq_put(&msgq_cloud, &msg, K_NO_WAIT);

		if (err)
		{
			LOG_ERR("Message could not be enqueued");
		}
	}

	return false;
}

static void cloud_event_handler(const struct aws_iot_evt *evt) 
{
switch (evt->type) {
	case AWS_IOT_EVT_CONNECTING: {
		struct cloud_module_event *event = new_cloud_module_event();
    	event->type = CLOUD_EVT_CONNECTING;
		APP_EVENT_SUBMIT(event);
		} break;
	case AWS_IOT_EVT_CONNECTED: 
		break;
	case AWS_IOT_EVT_READY: {
		struct cloud_module_event *event = new_cloud_module_event();
    	event->type = CLOUD_EVT_CONNECTED;
		APP_EVENT_SUBMIT(event);
		} break;
	case AWS_IOT_EVT_DISCONNECTED: {
		struct cloud_module_event *event = new_cloud_module_event();
    	event->type = CLOUD_EVT_DISCONNECTED;
		APP_EVENT_SUBMIT(event);
		} break;
	case AWS_IOT_EVT_DATA_RECEIVED: {
		
		if (is_topic(evt, TOPIC_UPDATE_DELTA)) {
			LOG_DBG("received delta update of length %d", evt->data.msg.len);

			struct cloud_module_event *event = new_cloud_module_event();
			event->type = CLOUD_EVT_UPDATE_DELTA;
			event->data.pub_msg.ptr = evt->data.msg.ptr;
			event->data.pub_msg.len = evt->data.msg.len;
			APP_EVENT_SUBMIT(event);
		}
		
	    } break;
	case AWS_IOT_EVT_PUBACK: {
		int err;
		err = qos_message_remove(evt->data.message_id);
		if (err == -ENODATA) {
			LOG_DBG("Message Acknowledgment not in pending QoS list, ID: %d",
				evt->data.message_id);
		} else if (err) {
			LOG_ERR("qos_message_remove, error: %d", err);
		}
		} break;
	case AWS_IOT_EVT_PINGRESP:
		break;
	case AWS_IOT_EVT_ERROR: {
		} break;
	default:
		LOG_ERR("Unknown AWS IoT event type: %d", evt->type);
		break;
	}
}

static void qos_event_handler(const struct qos_evt *evt)
{
	switch (evt->type) {
	case QOS_EVT_MESSAGE_NEW: {
		// LOG_DBG("QOS_EVT_MESSAGE_NEW");
		if (evt->message.type == CLOUD_SHADOW_UPDATE) {
			struct cloud_module_event *event = new_cloud_module_event();

			event->type = CLOUD_EVT_SEND_QOS;
			event->data.qos_msg = evt->message;

			APP_EVENT_SUBMIT(event);
		}

		if (evt->message.type == CLOUD_SHADOW_CLEAR) {
			struct cloud_module_event *event = new_cloud_module_event();

			event->type = CLOUD_EVT_SEND_QOS_CLEAR;
			event->data.qos_msg = evt->message;

			APP_EVENT_SUBMIT(event);
		}	
	}
		break;
	case QOS_EVT_MESSAGE_TIMER_EXPIRED: {
		LOG_DBG("QOS_EVT_MESSAGE_TIMER_EXPIRED");

		struct cloud_module_event *event = new_cloud_module_event();

		event->type = CLOUD_EVT_SEND_QOS;
		event->data.qos_msg = evt->message;

		APP_EVENT_SUBMIT(event);
	}
		break;
	case QOS_EVT_MESSAGE_REMOVED_FROM_LIST:
		// LOG_DBG("QOS_EVT_MESSAGE_REMOVED_FROM_LIST");

		if (evt->message.heap_allocated) {
			LOG_DBG("Freeing pointer: %p", evt->message.data.buf);
			k_free(evt->message.data.buf);
		}
		break;
	default:
		LOG_DBG("Unknown QoS handler event");
		break;
	}
}

/* Static module functions. */
static int setup(void)
{
	int err;

	err =  aws_iot_init(NULL, cloud_event_handler);
	if (err) {
		LOG_ERR("Failed initializing aws, error: %d", err);
	}

	err = qos_init(qos_event_handler);
	if (err) {
		LOG_ERR("qos_init, error: %d", err);
		return err;
	}

	

	return err;
}

static void connect_cloud(void)
{
	int err;
	int backoff_sec = backoff_delay[connect_retries].delay;

	LOG_DBG("Connecting to cloud");

	if (connect_retries > CONFIG_CLOUD_CONNECT_RETRIES) {
		LOG_WRN("Too many failed cloud connection attempts");
		return;
	}
	/* The cloud will return error if cloud_wrap_connect() is called while
	 * the socket is polled on in the internal cloud thread or the
	 * cloud backend is the wrong state. We cannot treat this as an error as
	 * it is rather common that cloud_connect can be called under these
	 * conditions.
	 */
	err = aws_iot_connect(&aws_config);
	if (err) {
		LOG_ERR("cloud_connect failed, error: %d", err);
	}

	connect_retries++;

	LOG_INF("Cloud connection establishment in progress");
	LOG_INF("New connection attempt in %d seconds if not successful",
		backoff_sec);
	k_sem_give(&cloud_connected_sem);
	/* Start timer to check connection status after backoff */
	k_work_reschedule(&connect_check_work, K_SECONDS(backoff_sec));
}

static void disconnect_cloud(void)
{
	int err;

	err = aws_iot_disconnect();
	if (err) {
		LOG_ERR("aws_iot_disconnect, error: %d", err);
		return;
	}

	connect_retries = 0;
	qos_timer_reset();

	k_work_cancel_delayable(&connect_check_work);
}

/* Convenience function used to add messages to the QoS library. */
static void add_qos_message(uint8_t *ptr, size_t len, uint8_t type,
			    uint32_t flags, bool heap_allocated)
{
	int err;
	struct qos_data message = {
		.heap_allocated = heap_allocated,
		.data.buf = ptr,
		.data.len = len,
		.id = qos_message_id_get_next(),
		.type = type,
		.flags = flags
	};

	err = qos_message_add(&message);
	if (err == -ENOMEM) {
		LOG_WRN("Cannot add message, internal pending list is full");
	} else if (err) {
		LOG_ERR("qos_message_add, error: %d", err);
	}
}

/* If this work is executed, it means that the connection attempt was not
 * successful before the backoff timer expired. A timeout message is then
 * added to the message queue to signal the timeout.
 */
static void connect_check_work_fn(struct k_work *work)
{
	// If cancelling works fails
	if ((state == STATE_LTE_CONNECTED && sub_state == SUB_STATE_CLOUD_CONNECTED) ||
		(state == STATE_LTE_DISCONNECTED)) {
		return;
	}

	LOG_DBG("Cloud connection timeout occurred");
	struct cloud_module_event *event = new_cloud_module_event();
	event->type = CLOUD_EVT_CONNECTION_TIMEOUT;
	APP_EVENT_SUBMIT(event);
}

/* Message handler for STATE_LTE_DISCONNECTED. */
static void on_state_lte_disconnected(struct cloud_msg_data *msg)
{
	if (is_modem_module_event((struct app_event_header *)(&msg->event.modem)))
    {
        if (msg->event.modem.type == MODEM_EVT_LTE_CONNECTED)
        {
			state_set(STATE_LTE_CONNECTED);

			/* LTE is now connected, cloud connection can be attempted */
			connect_cloud();
		}
	}
}

/* Message handler for STATE_LTE_CONNECTED. */
static void on_state_lte_connected(struct cloud_msg_data *msg)
{	
	if (is_modem_module_event((struct app_event_header *)(&msg->event.modem)))
    {
        if (msg->event.modem.type == MODEM_EVT_LTE_DISCONNECTED)
        {
			sub_state_set(SUB_STATE_CLOUD_DISCONNECTED);
			state_set(STATE_LTE_DISCONNECTED);

			/* Explicitly disconnect cloud when you receive an LTE disconnected event.
			* This is to clear up the cloud library state.
			*/
			disconnect_cloud();
		}
	}
}

/* Message handler for STATE_LTE_CONNECTED. */
static void on_sub_state_cloud_disconnected(struct cloud_msg_data *msg)
{
	if (is_cloud_module_event((struct app_event_header *)(&msg->event.cloud)))
    {
        if (msg->event.cloud.type == CLOUD_EVT_CONNECTED)
        {
			sub_state_set(SUB_STATE_CLOUD_CONNECTED);
			LOG_INF("Cloud connected");

			connect_retries = 0;
			k_work_cancel_delayable(&connect_check_work);
		}
	}

	if (is_cloud_module_event((struct app_event_header *)(&msg->event.cloud)))
    {
        if (msg->event.cloud.type == CLOUD_EVT_CONNECTION_TIMEOUT)
        {
			connect_cloud();
		}
	}
}

/* Message handler for STATE_LTE_CONNECTED. */
static void on_sub_state_cloud_connected(struct cloud_msg_data *msg)
{
	int err = 0;

	if (is_robot_module_event((struct app_event_header *)(&msg->event.robot)))
    {
        if (msg->event.robot.type == ROBOT_EVT_REPORT)
        {
		// char *str = cloud_encode_add_robot(msg->event.robot.data.str);
		add_qos_message(msg->event.robot.data.str, 
				strlen(msg->event.robot.data.str),
				CLOUD_SHADOW_UPDATE,
				QOS_FLAG_RELIABILITY_ACK_REQUIRED,
				false);
		}
	}

	if (is_cloud_module_event((struct app_event_header *)(&msg->event.cloud)))
    {
        if (msg->event.cloud.type == CLOUD_EVT_SEND_QOS)
        {
			struct qos_payload *qos_payload = &msg->event.cloud.data.qos_msg.data;

			struct aws_iot_data message = {
				.ptr = qos_payload->buf,
				.len = qos_payload->len,
				.message_id = msg->event.cloud.data.qos_msg.id,
				.qos = MQTT_QOS_1_AT_LEAST_ONCE,
				.topic.type = AWS_IOT_SHADOW_TOPIC_UPDATE,
			};
			LOG_DBG("Sending payload: %s", qos_payload->buf);
			err = aws_iot_send(&message);
			if (err) {
				LOG_ERR("aws_iot_send, error: %d", err);
			}
		}
	}
	
	if (is_cloud_module_event((struct app_event_header *)(&msg->event.cloud)))
    {
        if (msg->event.cloud.type == CLOUD_EVT_SEND_QOS_CLEAR)
        {
			struct qos_payload *qos_payload = &msg->event.cloud.data.qos_msg.data;

			struct aws_iot_data message = {
				.ptr = qos_payload->buf,
				.len = qos_payload->len,
				.message_id = msg->event.cloud.data.qos_msg.id,
				.qos = MQTT_QOS_1_AT_LEAST_ONCE,
				.topic.type = AWS_IOT_SHADOW_TOPIC_DELETE,
			};

			err = aws_iot_send(&message);
			if (err) {
				LOG_ERR("aws_iot_send, error: %d", err);
			}
		}
	}
}

static void module_thread_fn(void)
{
	int err;
	struct cloud_msg_data msg;

	LOG_INF("Cloud module thread started");


	err = setup();
	if (err) {
		LOG_ERR("Failed setting up the cloud, error: %d", err);
	}

	k_work_init_delayable(&connect_check_work, connect_check_work_fn);

	while (true) {
		k_msgq_get(&msgq_cloud, &msg, K_FOREVER);

		switch (state) {
		case STATE_LTE_DISCONNECTED:
			on_state_lte_disconnected(&msg);
			break;
		case STATE_LTE_CONNECTED:
			switch (sub_state)
			{
			case SUB_STATE_CLOUD_DISCONNECTED:
				on_sub_state_cloud_disconnected(&msg);
				break;
			case SUB_STATE_CLOUD_CONNECTED:
				on_sub_state_cloud_connected(&msg);
				break;
			
			default:
				break;
			}
			
			on_state_lte_connected(&msg);
			break;
		default:
			break;
		}
	}
}

static void aws_poll_thread_fn(void)
{
	int err;
	LOG_INF("polling thread started");
	k_sem_take(&cloud_connected_sem, K_FOREVER);
	struct pollfd fds[] = {
		{
			.fd = aws_config.socket,
			.events = POLLIN
		}
	};

	while (true) {
		err = poll(fds, ARRAY_SIZE(fds),4000);
		if (err < 0) {
			LOG_ERR("poll() returned an error: %d", err);
			continue;
		}

		if (err == 0) {
			aws_iot_ping();
			continue;
		}

		if ((fds[0].revents & POLLIN) == POLLIN) {
			aws_iot_input();
		}

		if ((fds[0].revents & POLLNVAL) == POLLNVAL) {
			LOG_ERR("Socket error: POLLNVAL");
			LOG_ERR("The AWS IoT socket was unexpectedly closed.");
			return;
		}

		if ((fds[0].revents & POLLHUP) == POLLHUP) {
			LOG_ERR("Socket error: POLLHUP");
			LOG_ERR("Connection was closed by the AWS IoT broker.");
			return;
		}

		if ((fds[0].revents & POLLERR) == POLLERR) {
			LOG_ERR("Socket error: POLLERR");
			LOG_ERR("AWS IoT broker connection was unexpectedly closed.");
			return;
		}
	}
}

K_THREAD_DEFINE(cloud_module_thread, CONFIG_CLOUD_THREAD_STACK_SIZE,
		module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

K_THREAD_DEFINE(aws_poll_thread, CONFIG_CLOUD_THREAD_STACK_SIZE,
		aws_poll_thread_fn, NULL, NULL, NULL,
		K_HIGHEST_APPLICATION_THREAD_PRIO, 0, 0);
		
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, cloud_module_event);
APP_EVENT_SUBSCRIBE(MODULE, modem_module_event);
APP_EVENT_SUBSCRIBE(MODULE, robot_module_event);


