/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <modem/lte_lc.h>

#define MODULE modem_module

// #include "modules_common.h"
#include "modem_module_event.h"
#include "ui_module_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_MODEM_MODULE_LOG_LEVEL);

/* Modem module super states. */
static enum state_type {
	STATE_DISCONNECTED,
	STATE_CONNECTING,
	STATE_CONNECTED,
} state;

struct modem_msg_data {
	union {
		struct modem_module_event modem;
		struct ui_module_event ui;
	} event;
};

/* Modem module message queue. */
#define MODEM_QUEUE_ENTRY_COUNT		10
#define MODEM_QUEUE_BYTE_ALIGNMENT	4

K_MSGQ_DEFINE(msgq_modem, sizeof(struct modem_msg_data),
	      MODEM_QUEUE_ENTRY_COUNT, MODEM_QUEUE_BYTE_ALIGNMENT);

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type state)
{
	switch (state) {
	case STATE_DISCONNECTED:
		return "STATE_DISCONNECTED";
	case STATE_CONNECTING:
		return "STATE_CONNECTING";
	case STATE_CONNECTED:
		return "STATE_CONNECTED";
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
	struct modem_msg_data msg = {0};
	bool enqueue_msg = false;

	if (is_modem_module_event(aeh)) {
		struct modem_module_event *evt = cast_modem_module_event(aeh);
		msg.event.modem = *evt;
		enqueue_msg = true;
	}

	if (is_ui_module_event(aeh)) {
		struct ui_module_event *evt = cast_ui_module_event(aeh);

		msg.event.ui = *evt;
		enqueue_msg = true;
	}

	if (enqueue_msg)
	{
		int err = k_msgq_put(&msgq_modem, &msg, K_NO_WAIT);

		if (err)
		{
			LOG_ERR("Message could not be enqueued");
		}
	}

	return false;
}

static void lte_evt_handler(const struct lte_lc_evt *const evt)
{
	
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS: {
		if (evt->nw_reg_status == LTE_LC_NW_REG_NOT_REGISTERED) {
			struct modem_module_event *event = new_modem_module_event();
			event->type = MODEM_EVT_LTE_DISCONNECTED;
			APP_EVENT_SUBMIT(event);
			break;
		}

		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
			(evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
				break;
		}

        LOG_INF("Connected to %s network",
             evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ? "home" : "roaming");
			struct modem_module_event *event = new_modem_module_event();
			event->type = MODEM_EVT_LTE_CONNECTED;
			APP_EVENT_SUBMIT(event);
		break;
	}
	default:
		break;
	}
}

static int lte_connect(void)
{
	int err;

	err = lte_lc_connect_async(lte_evt_handler);
	if (err) {
		LOG_ERR("lte_lc_connect_async, error: %d", err);

		return err;
	}
	struct modem_module_event *event = new_modem_module_event();
	event->type = MODEM_EVT_LTE_CONNECTING;
	APP_EVENT_SUBMIT(event);
	return 0;
}

/* Static module functions. */
static int setup(void)
{
	int err;

	err = lte_lc_init();
	if (err) {
		LOG_ERR("lte_lc_init, error: %d", err);
		return err;
	}

	err = lte_connect();
	if (err) {
		LOG_ERR("Failed connecting to LTE, error: %d", err);
		return err;
	}

	return err;
}

static int lte_disconnect(void)
{
	int err;

	err = lte_lc_offline();
	if (err) {
		LOG_ERR("failed to go offline, error: %d", err);

		return err;
	}

	return 0;
}

/* Message handler for STATE_DISCONNECTED. */
static void on_state_disconnected(struct modem_msg_data *msg)
{
	if (is_modem_module_event((struct app_event_header *)(&msg->event.modem)))
    {
        if (msg->event.modem.type == MODEM_EVT_LTE_CONNECTING)
        {
			state_set(STATE_CONNECTING);
		}
	}
}

/* Message handler for STATE_CONNECTING. */
static void on_state_connecting(struct modem_msg_data *msg)
{
	if (is_modem_module_event((struct app_event_header *)(&msg->event.modem)))
    {
        if (msg->event.modem.type == MODEM_EVT_LTE_CONNECTED)
        {
			state_set(STATE_CONNECTED);
		}
	}
}

/* Message handler for STATE_CONNECTED. */
static void on_state_connected(struct modem_msg_data *msg)
{
	if (is_modem_module_event((struct app_event_header *)(&msg->event.modem)))
    {
        if (msg->event.modem.type == MODEM_EVT_LTE_DISCONNECTED)
        {
			state_set(STATE_DISCONNECTED);
		}
	}
}

static void module_thread_fn(void)
{
	int err;
	LOG_INF("Modem module thread started");
	struct modem_msg_data msg;

	err = setup();
	if (err) {
		LOG_ERR("Failed setting up the modem, error: %d", err);
	}

	while (true) {
		// module_get_next_msg(&self, &msg);
		k_msgq_get(&msgq_modem, &msg, K_FOREVER);

		switch (state) {
		case STATE_DISCONNECTED:
			on_state_disconnected(&msg);
			break;
		case STATE_CONNECTING:
			on_state_connecting(&msg);
			break;
		case STATE_CONNECTED:
			on_state_connected(&msg);
			break;
		default:
			break;
		}
	}
}

K_THREAD_DEFINE(modem_module_thread, CONFIG_MODEM_THREAD_STACK_SIZE,
		module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, modem_module_event);
APP_EVENT_SUBSCRIBE(MODULE, ui_module_event);


