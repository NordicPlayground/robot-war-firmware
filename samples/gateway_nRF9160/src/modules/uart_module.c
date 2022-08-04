/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>

#define MODULE uart_module
#include "modules_common.h"
#include "uart_module_event.h"
#include "mesh_module_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_UART_MODULE_LOG_LEVEL);

struct uart_msg_header {
    uint32_t len;
    uint32_t type;
    uint32_t id;
	uint32_t addr;
};

// struct uart_msg_header {
//     uint8_t len;
//     uint32_t type;
// 	uint16_t addr;
// };

struct uart_msg {
    struct uart_msg_header header;
    uint8_t *data;
};

struct uart_msg_data
{
	union
	{
		struct uart_module_event uart;
		struct mesh_module_event mesh;
	} module;
};

/* Uart module message queue. */
#define UART_QUEUE_ENTRY_COUNT		10
#define UART_QUEUE_BYTE_ALIGNMENT	4

K_MSGQ_DEFINE(msgq_uart, sizeof(struct uart_msg_data),
	      UART_QUEUE_ENTRY_COUNT, UART_QUEUE_BYTE_ALIGNMENT);

static uint8_t rx_buf[2];

static struct k_fifo rx_fifo;
static struct k_fifo tx_fifo;

/* uart module super states. */
static enum state_type {
	STATE_UART_NOT_READY,
	STATE_UART_READY,
} state;

/* Uart module devices and static module data. */
static K_SEM_DEFINE(uart_tx_sem, 1, 1);

static const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart2));

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type state)
{
	switch (state)
	{
	case STATE_UART_NOT_READY:
		return "STATE_UART_NOT_READY";
	case STATE_UART_READY:
		return "STATE_UART_READY";
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
	struct uart_msg_data msg = {0};
	bool enqueue_msg = false;

	if (is_uart_module_event(aeh))
	{
		struct uart_module_event *evt = cast_uart_module_event(aeh);
		msg.module.uart = *evt;
		enqueue_msg = true;
	}

	if (is_mesh_module_event(aeh))
	{
		struct mesh_module_event *evt = cast_mesh_module_event(aeh);
		msg.module.mesh = *evt;
		enqueue_msg = true;
	}

	if (enqueue_msg)
	{
		int err = k_msgq_put(&msgq_uart, &msg, K_NO_WAIT);

		if (err)
		{
			LOG_ERR("Message could not be enqueued");
		}
	}

	return false;
}

int uart_send(const struct device *dev, uint8_t *data, uint32_t len, uint32_t type, uint32_t id, uint32_t addr)
{
	uint8_t *pos;
	uint8_t *msg = k_malloc(len + sizeof(struct uart_msg_header));
	if (!msg){
		LOG_ERR("Unable to allocate msg");
		return -ENOMEM;
	}

	pos = msg;
	memcpy(pos, &len, sizeof(uint32_t));

	pos = pos + sizeof(uint32_t);
	memcpy(pos, &type, sizeof(uint32_t));

	pos = pos + sizeof(uint32_t);
	memcpy(pos, &id, sizeof(uint32_t));

	pos = pos + sizeof(uint32_t);
	memcpy(pos, &addr, sizeof(uint32_t));

	if (data != NULL) {
		pos = pos + sizeof(uint32_t);
		memcpy(pos, data, len);
	}

	LOG_HEXDUMP_INF(msg, len + sizeof(struct uart_msg_header), "message:");
	k_fifo_put(&tx_fifo, msg);
	
	return 0;
}

static int uart_data_rx_rdy_handler(const struct device *dev, struct uart_event_rx event_data)
{
	static struct uart_msg *msg;
	static uint8_t *msg_buf;
	static uint8_t *data;
	static size_t current_msg_len = 0;
	static size_t expected_data_len = 0;
	uint8_t header_size = sizeof(struct uart_msg_header);

	if (current_msg_len == 0) {
		/* New message just started, read expected length and allocate data buffer */
		expected_data_len = *(event_data.buf);
		
		msg = (struct uart_msg *)k_malloc(sizeof(struct uart_msg));
		if (!msg){
			LOG_ERR("Unable to allocate msg");
		}
		data = k_malloc(expected_data_len);
		if (!data){
			LOG_ERR("Unable to allocate msg data buffer");
		}
		msg_buf = (uint8_t*)msg;
		msg->data = data;
	} 
	if (current_msg_len < header_size) {
		/* Second byte received, read type of message */
		memcpy((void*)(msg_buf + (current_msg_len)), (event_data.buf), 1);
	} else {
		/* Data byte received, store in message*/
		memcpy((void*)(data + (current_msg_len-header_size)), (event_data.buf), 1);
	}

	current_msg_len ++;
	
	if (current_msg_len == (expected_data_len + sizeof(struct uart_msg_header))) {
		/* Mesagge is complete, add to fifo */
		current_msg_len = 0;
		k_fifo_put(&rx_fifo, msg_buf);
		return 0;
	}
	
	/* Continue receiving */
	return 0;
}

static void uart_callback(const struct device *dev, struct uart_event *event, void *user_data)
{
	static uint8_t *released_buf = &rx_buf[1];
	// struct uart_msg_handlers *handlers = (struct uart_msg_handlers *)user_data;
	switch (event->type)
	{
	case UART_TX_DONE:
	{
		LOG_DBG("UART_TX_DONE: Sent %d bytes", event->data.tx.len);
		// if (handlers) {
		// 	if (handlers->tx_done){
		// 		handlers->tx_done(dev, event->data.tx.buf, event->data.tx.len);
		// 	}
		// }
		k_free((void*)event->data.tx.buf);
		k_sem_give(&uart_tx_sem);
		break;
	}
	case UART_TX_ABORTED:
	{
		LOG_ERR("UART_TX_ABORTED");
		k_sem_give(&uart_tx_sem);
		break;
	}
	case UART_RX_RDY:
	{
		LOG_DBG("UART_RX_RDY");
		uart_data_rx_rdy_handler(dev, event->data.rx);
		break;
	}
	case UART_RX_BUF_REQUEST:
	{
		LOG_DBG("UART_RX_BUF_REQUEST");
		if (released_buf == &rx_buf[0]) {
			uart_rx_buf_rsp(dev, &rx_buf[0], 1);
		} else {
			uart_rx_buf_rsp(dev, &rx_buf[1], 1);
		}
		break;
	}
	case UART_RX_BUF_RELEASED:
	{
		LOG_DBG("UART_RX_BUF_RELEASED");
		released_buf = event->data.rx_buf.buf;
		break;
	}
	case UART_RX_DISABLED:
	{
		LOG_DBG("UART_RX_DISABLED");
		break;
	}
	case UART_RX_STOPPED:
	{
		LOG_DBG("UART_RX_STOPPED: Reason: %d", event->data.rx_stop.reason);
		break;
	}
	default:
		LOG_ERR("Unknown UART event type %d", event->type);
		break;
	}
}

static int init_uart()
{
	int err = 0;
	const struct gpio_dt_spec reset_gpio = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(nrf52840_reset), gpios, 0);
	if (!device_is_ready(reset_gpio.port))
	{
		LOG_ERR("nRF52840 Reset GPIO not ready");
		return -ENODEV;
	}
	gpio_pin_configure_dt(&reset_gpio, GPIO_OUTPUT_INACTIVE);
	gpio_pin_set_dt(&reset_gpio, 1);
	k_sleep(K_MSEC(10));
	gpio_pin_set_dt(&reset_gpio, 0);
	k_sleep(K_MSEC(1000)); // Wait for UART on nRF52840 to be ready.

	if (!device_is_ready(uart))
	{
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}
	LOG_DBG("UART device ready");

	err = uart_callback_set(uart, uart_callback, NULL);
	if (err)
	{
		LOG_ERR("Failed to set UART callback: Error %d", err);
		return err;
	}



	uart_rx_enable(uart, &rx_buf[0], 1, SYS_FOREVER_US);
	return 0;
}

/* Module state handlers. */
// static void on_state_uart_ready(struct uart_msg_data *msg)
// {
// 	if (IS_EVENT(msg, uart, UART_EVT_RX)) {
// 		k_free(msg->module.uart.data.rx.data);
// 	}

// 	if (IS_EVENT(msg, uart, UART_EVT_TX)) {
// 		LOG_INF("sending!");
// 		uart_send(uart, msg->module.uart.data.tx.data, 
// 				msg->module.uart.data.tx.header.len,
// 				msg->module.uart.data.tx.header.type,
// 				msg->module.uart.data.tx.header.id,
// 				msg->module.uart.data.tx.header.addr);
// 	}
// }

static void on_all_states(struct uart_msg_data *msg)
{
	if (IS_EVENT(msg, uart, UART_EVT_RX)) {
		k_free(msg->module.uart.data.rx.data);
	}

	if (IS_EVENT(msg, uart, UART_EVT_TX)) {
		LOG_INF("msg->module.uart.data.tx.data %p", msg->module.uart.data.tx.data);
		LOG_HEXDUMP_INF((uint8_t*)(msg->module.uart.data.tx.data), msg->module.uart.data.tx.header.len, "payload:");
		uart_send(uart, (uint8_t*)msg->module.uart.data.tx.data, 
				msg->module.uart.data.tx.header.len,
				msg->module.uart.data.tx.header.type,
				msg->module.uart.data.tx.header.id,
				msg->module.uart.data.tx.header.addr);
	}
}

static void module_thread_fn(void)
{
	LOG_INF("Uart module thread started");
	state_set(STATE_UART_NOT_READY);

	int err = init_uart();
	if (err)
	{
		LOG_ERR("Could not initialize UART: Error %d", err);
		return;
	}
	LOG_DBG("UART initialized");

	struct uart_msg_data msg = {0};
	while (true)
	{
		k_msgq_get(&msgq_uart, &msg, K_FOREVER);

		// switch (state)
		// {
		// case STATE_UART_READY:
		// {
		// 	on_state_uart_ready(&msg);
		// 	break;
		// }
		// default:
		// 	break;
		// }

		on_all_states(&msg);
	}
}

static void uart_rx_thread_fn(void *arg1, void *arg2, void *arg3)
{
	LOG_DBG("Starting UART rx thread");
	struct uart_msg *msg;
	k_fifo_init(&rx_fifo);
	// const struct device *dev = (const struct device *)arg1;
	// struct uart_msg_handlers *handlers = (struct uart_msg_handlers *)arg2;
	while (true)
	{
		msg = k_fifo_get(&rx_fifo, K_FOREVER);
		struct uart_module_event *event = new_uart_module_event();
		event->type = UART_EVT_RX;
		event->data.rx.header.len = msg->header.len;
		event->data.rx.header.type = msg->header.type;
		event->data.rx.header.id = msg->header.id;
		event->data.rx.header.addr = msg->header.addr;
		event->data.rx.data = msg->data;
		APP_EVENT_SUBMIT(event);
		k_free(msg);
	}
}

static void uart_tx_thread_fn(void *arg1, void *arg2, void *arg3)
{
	LOG_DBG("Starting UART tx thread");
	int err;
	uint8_t *msg;
	size_t size;

	k_fifo_init(&tx_fifo);
	while (true)
	{
		msg = k_fifo_get(&tx_fifo, K_FOREVER);
		size = *msg + sizeof(struct uart_msg_header);

		err = uart_tx(uart, msg, size, SYS_FOREVER_US);
		if (err){
			LOG_ERR("Failed to send data: Error %d", err);
		}
		
		err = k_sem_take(&uart_tx_sem, K_FOREVER);
		if (err){
			LOG_ERR("Failed to take semaphore: Error %d", err);
		}
	}
}

K_THREAD_DEFINE(uart_rx_thread, CONFIG_UART_THREAD_STACK_SIZE, 
				uart_rx_thread_fn, NULL, NULL, NULL, 
				CONFIG_UART_RX_THREAD_PRIORITY, 0, 0);

K_THREAD_DEFINE(uart_tx_thread, CONFIG_UART_THREAD_STACK_SIZE, 
				uart_tx_thread_fn, NULL, NULL, NULL, 
				CONFIG_UART_TX_THREAD_PRIORITY, 0, 0);

K_THREAD_DEFINE(uart_module_thread, CONFIG_UART_THREAD_STACK_SIZE,
				module_thread_fn, NULL, NULL, NULL,
				K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, mesh_module_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, uart_module_event);