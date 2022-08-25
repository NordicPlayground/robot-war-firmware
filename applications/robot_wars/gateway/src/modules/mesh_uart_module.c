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

#define MODULE mesh_module
#include "mesh_module_event.h"
#include "robot_module_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_MESH_MODULE_LOG_LEVEL);

struct uart_msg_header {
    uint32_t len;
    uint32_t type;
    uint32_t id;
	uint32_t addr;
};

struct uart_msg {
	/** reserved for FIFO use. */
	void *fifo_reserved;
    struct uart_msg_header header;
    uint8_t *data;
};


// struct uart_tx_msg {
//     uint8_t *data;
// };

struct mesh_msg_data
{
	union {
		// struct uart_module_event uart;
		struct robot_module_event robot;
	} event;
};

/* Mesh module message queue. */
#define MESH_QUEUE_ENTRY_COUNT		10
#define MESH_QUEUE_BYTE_ALIGNMENT	4

K_MSGQ_DEFINE(msgq_mesh, sizeof(struct mesh_msg_data),
	      MESH_QUEUE_ENTRY_COUNT, MESH_QUEUE_BYTE_ALIGNMENT);

/* Uart module devices and static module data. */
static uint8_t rx_buf[2];

static struct k_fifo rx_fifo;
static struct k_fifo tx_fifo;

static K_SEM_DEFINE(uart_tx_sem, 0, 1);

static const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart2));

// int uart_send(const struct device *dev, uint8_t *data, uint32_t len, uint32_t type, uint32_t id, uint32_t addr)
// {
// 	uint8_t *pos;
// 	// uint8_t *msg_buf;
// 	struct uart_tx_msg *msg = k_malloc(sizeof(struct uart_tx_msg));
// 	if (!msg){
// 		LOG_ERR("Unable to allocate msg");
// 		return -ENOMEM;
// 	}

// 	uint8_t *msg_data = k_malloc(len + sizeof(struct uart_msg_header));
// 	if (!data){
// 		LOG_ERR("Unable to allocate msg data");
// 		return -ENOMEM;
// 	}

// 	msg->data = msg_data;
	
// 	// msg_buf = (uint8_t*)msg;
// 	pos = msg_data;
// 	memcpy(pos, &len, sizeof(uint32_t));

// 	pos += sizeof(uint32_t);
// 	memcpy(pos, &type, sizeof(uint32_t));

// 	pos += sizeof(uint32_t);
// 	memcpy(pos, &id, sizeof(uint32_t));

// 	pos += sizeof(uint32_t);
// 	memcpy(pos, &addr, sizeof(uint32_t));

// 	if (data != NULL) {
// 		pos += sizeof(uint32_t);
// 		memcpy(pos, data, len);
// 	}
// 	// LOG_INF("pointer send %p, len: %d", msg, *msg);

// 	LOG_HEXDUMP_INF(msg->data, len + sizeof(struct uart_msg_header), "message:");
// 	k_fifo_put(&tx_fifo, msg);
	
// 	return 0;
// }

int uart_send(const struct device *dev, uint8_t *data, uint32_t len, uint32_t type, uint32_t id, uint32_t addr)
{
	// uint8_t *pos;
	// uint8_t *msg_buf;
	struct uart_msg *msg = k_malloc(sizeof(struct uart_msg));
	if (!msg){
		LOG_ERR("Unable to allocate msg");
		return -ENOMEM;
	}

	uint8_t *msg_data = k_malloc(len);
	if (!msg_data){
		LOG_ERR("Unable to allocate msg data");
		return -ENOMEM;
	}

	msg->header.addr = addr;
	msg->header.id = id;
	msg->header.len = len;
	msg->header.type = type;
	msg->data = msg_data;

	if (data != NULL) {
		memcpy(msg_data, data, len);
	}
	// LOG_INF("pointer send %p, len: %d", msg, msg->header.len);

	// LOG_HEXDUMP_INF(msg->data, len, "message:");
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
		// LOG_INF("expected data len %d", expected_data_len);
		
		msg = (struct uart_msg *)k_malloc(sizeof(struct uart_msg));
		if (!msg){
			LOG_ERR("Unable to allocate msg");
		}
		data = k_malloc(expected_data_len);
		if (!data){
			LOG_ERR("Unable to allocate msg data buffer");
		}
		msg_buf = (uint8_t*)(&msg->header);
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
		k_fifo_put(&rx_fifo, msg);
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
		// LOG_INF("UART_TX_DONE: Sent %d bytes", event->data.tx.len);
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
		LOG_INF("UART_TX_ABORTED");
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

/* Event handlers */
static bool app_event_handler(const struct app_event_header *aeh)
{
	struct mesh_msg_data msg = {0};
	bool enqueue_msg = false;

	if (is_robot_module_event(aeh))
	{
		struct robot_module_event *evt = cast_robot_module_event(aeh);
		msg.event.robot = *evt;
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

static void on_all_states(struct mesh_msg_data *msg)
{
	if (is_robot_module_event((struct app_event_header *)(&msg->event.robot)))
    {
		if (msg->event.robot.type == ROBOT_EVT_MOVEMENT_CONFIGURE)
        {	
			// LOG_INF("movement: time: %d, rotation %d, speed %d", msg->event.robot.data.movement->drive_time, msg->event.robot.data.movement->rotation, msg->event.robot.data.movement->speed);
			uart_send(uart, (uint8_t*)msg->event.robot.data.movement, 9,
					BT_MESH_MOVEMENT_OP_MOVEMENT_SET, MOVEMENT_CLI_MODEL_ID,
					msg->event.robot.addr);
		}
	}

	if (is_robot_module_event((struct app_event_header *)(&msg->event.robot)))
    {
		if (msg->event.robot.type == ROBOT_EVT_CLEAR_TO_MOVE)
        {	
			uart_send(uart, NULL, 0, BT_MESH_MOVEMENT_OP_READY_SET, 
				MOVEMENT_CLI_MODEL_ID, 0xFFFF);
		}
	}
	
	if (is_robot_module_event((struct app_event_header *)(&msg->event.robot)))
    {
		if (msg->event.robot.type == ROBOT_EVT_LED_CONFIGURE)
        {	
			// LOG_INF("LED event!");
			uart_send(uart, (uint8_t*)msg->event.robot.data.led, 5,
					BT_MESH_LIGHT_RGB_OP_RGB_SET, LIGHT_RGB_CLI_MODEL_ID,
					msg->event.robot.addr);
		}
	}
}

static void module_thread_fn(void)
{
	LOG_INF("Mesh module thread started");
	
	int err = init_uart();
	if (err)
	{
		LOG_ERR("Could not initialize UART: Error %d", err);
		return;
	}
	LOG_DBG("UART initialized");

	struct mesh_msg_data msg = {0};
	while (true)
	{
		k_msgq_get(&msgq_mesh, &msg, K_FOREVER);
		
		on_all_states(&msg);
	}
}

static void uart_rx_thread_fn(void *arg1, void *arg2, void *arg3)
{
	LOG_DBG("Starting UART rx thread");
	struct uart_msg *msg;
	k_fifo_init(&rx_fifo);
	struct mesh_module_event *event;
	while (true)
	{
		msg = k_fifo_get(&rx_fifo, K_FOREVER);
		// LOG_INF("rx thread!");
		event = new_mesh_module_event();
		switch (msg->header.id)
		{
		case ID_CLI_MODEL_ID:
			if(msg->header.type == BT_MESH_ID_OP_STATUS)
			{
				event->type = MESH_EVT_ROBOT_ID;
				struct bt_mesh_id_status id;
				memcpy((void*)&id.id, msg->data, 6);
				event->data.robot_id = id;
				event->addr = msg->header.addr;
			} 
			break;
		case MOVEMENT_CLI_MODEL_ID:
			if(msg->header.type == BT_MESH_MOVEMENT_OP_MOVEMENT_ACK)
			{
				event->type = MESH_EVT_MOVEMENT_CONFIGURED;
				event->addr = msg->header.addr;
			} 
			break;
		case TELEMETRY_CLI_MODEL_ID:
			if(msg->header.type == BT_MESH_TELEMETRY_OP_TELEMETRY_REPORT)
			{
				event->type = MESH_EVT_TELEMETRY_REPORTED;
				event->addr = msg->header.addr;
				struct bt_mesh_telemetry_report report;
				memcpy((void*)&report.revolutions, msg->data, 1);
				event->data.report = report;
			}
			break;		
		default:
			break;
		}
		
		APP_EVENT_SUBMIT(event);
		k_free(msg->data);
		k_free(msg);
	}
}

static void uart_tx_thread_fn(void *arg1, void *arg2, void *arg3)
{
	LOG_DBG("Starting UART tx thread");
	int err;
	// uint8_t *msg;
	struct uart_msg *msg;
	size_t size;
	uint8_t *uart_packet;

	k_fifo_init(&tx_fifo);
	while (true)
	{
		msg = k_fifo_get(&tx_fifo, K_FOREVER);
		size = msg->header.len + sizeof(struct uart_msg_header);
		uart_packet = k_malloc(msg->header.len + sizeof(struct uart_msg_header));
		memcpy(uart_packet, &msg->header, sizeof(struct uart_msg_header));
		memcpy(uart_packet + sizeof(struct uart_msg_header), msg->data, msg->header.len);
		// k_free(msg);
		// size = *msg->data + sizeof(struct uart_msg_header);
		// LOG_INF("pointer thread %p, data %p", msg, msg->data);
		LOG_HEXDUMP_INF(uart_packet, size, "tx message:");

		err = uart_tx(uart, uart_packet, size, SYS_FOREVER_US);
		if (err){
			LOG_ERR("Failed to send data: Error %d", err);
		}
		
		err = k_sem_take(&uart_tx_sem, K_FOREVER);
		if (err){
			LOG_ERR("Failed to take semaphore: Error %d", err);
		}
		
		k_free(msg->data);
		k_free(msg);
	}
}

K_THREAD_DEFINE(mesh_module_thread, CONFIG_MESH_THREAD_STACK_SIZE,
				module_thread_fn, NULL, NULL, NULL,
				K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

K_THREAD_DEFINE(uart_rx_thread, CONFIG_UART_RX_THREAD_STACK_SIZE, 
				uart_rx_thread_fn, NULL, NULL, NULL, 
				CONFIG_UART_RX_THREAD_PRIORITY, 0, 0);

K_THREAD_DEFINE(uart_tx_thread, CONFIG_UART_TX_THREAD_STACK_SIZE, 
				uart_tx_thread_fn, NULL, NULL, NULL, 
				CONFIG_UART_TX_THREAD_PRIORITY, 0, 0);

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, robot_module_event);
APP_EVENT_SUBSCRIBE(MODULE, uart_module_event);