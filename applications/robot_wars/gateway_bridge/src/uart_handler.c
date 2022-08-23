#include <zephyr.h>
#include "uart_handler.h"

#define MODULE uart

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_UART_MODULE_LOG_LEVEL);

struct uart_msg_header {
    uint32_t len;
    uint32_t type;
    uint32_t id;
	uint32_t addr;
};

struct uart_msg {
    struct uart_msg_header header;
    uint8_t *data;
};

static uint8_t rx_buf[2];

static struct k_fifo rx_fifo;
static struct k_fifo tx_fifo;

static K_SEM_DEFINE(uart_tx_sem, 0, 1);
K_THREAD_STACK_DEFINE(uart_rx_thread_stack, CONFIG_UART_RX_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(uart_tx_thread_stack, CONFIG_UART_TX_THREAD_STACK_SIZE);
static struct k_thread uart_rx_thread_thread;
static struct k_thread uart_tx_thread_thread;

static void uart_rx_thread_fn(void *arg1, void *arg2, void *arg3);
static void uart_tx_thread_fn(void *arg1, void *arg2, void *arg3);

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
		msg_buf = (uint8_t*)msg;

		if (expected_data_len != 0) {
			data = k_malloc(expected_data_len);
			if (!data){
				LOG_ERR("Unable to allocate msg data buffer");
			}
			msg->data = data;
		} else {
			msg->data = NULL;
		}

		
		
	} 
	if (current_msg_len < header_size) {
		/* Second byte received, read type of message */
		memcpy((void*)(msg_buf + (current_msg_len)), (event_data.buf), 1);
	} else if (expected_data_len != 0) {
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
	struct uart_msg_handlers *handlers = (struct uart_msg_handlers *)user_data;
	switch (event->type)
	{
	case UART_TX_DONE:
	{
		LOG_DBG("UART_TX_DONE: Sent %d bytes", event->data.tx.len);
		if (handlers) {
			if (handlers->tx_done){
				handlers->tx_done(dev, event->data.tx.buf, event->data.tx.len);
			}
		}
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

int uart_send(const struct device *dev, uint8_t *data, uint32_t len, uint32_t type, uint32_t id, uint32_t addr)
{
	uint8_t *pos;
	uint8_t *msg = k_malloc(len + sizeof(struct uart_msg));
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

	pos = pos + sizeof(uint32_t);
	memcpy(pos, data, len);
	// LOG_HEXDUMP_INF(data, len, "message:");
	k_fifo_put(&tx_fifo, msg);
	
	return 0;
}

int init_uart(const struct device *uart_dev, struct uart_msg_handlers *handlers)
{
	int err = 0;
	k_thread_create(&uart_rx_thread_thread, uart_rx_thread_stack, 
		CONFIG_UART_RX_THREAD_STACK_SIZE, uart_rx_thread_fn, 
		(void*)uart_dev, handlers, NULL, 
		CONFIG_UART_RX_THREAD_PRIORITY, 0, K_NO_WAIT);
	
	k_thread_create(&uart_tx_thread_thread, uart_tx_thread_stack, 
		CONFIG_UART_TX_THREAD_STACK_SIZE, uart_tx_thread_fn, 
		(void*)uart_dev, NULL, NULL, 
		CONFIG_UART_TX_THREAD_PRIORITY, 0, K_NO_WAIT);

	if (!device_is_ready(uart_dev))
	{
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}
	LOG_DBG("UART device ready");

	err = uart_callback_set(uart_dev, uart_callback, handlers);
	if (err)
	{
		LOG_ERR("Failed to set UART callback: Error %d", err);
		return err;
	}

	k_fifo_init(&rx_fifo);
	k_fifo_init(&tx_fifo);

	uart_rx_enable(uart_dev, &rx_buf[0], 1, SYS_FOREVER_US);
	return 0;
}

static void uart_rx_thread_fn(void *arg1, void *arg2, void *arg3)
{
	LOG_DBG("Starting UART rx thread");
	struct uart_msg *msg;
	const struct device *dev = (const struct device *)arg1;
	struct uart_msg_handlers *handlers = (struct uart_msg_handlers *)arg2;
	while (true)
	{
		msg = k_fifo_get(&rx_fifo, K_FOREVER);
		LOG_INF("rx_thread");
		if (handlers) {
			if (handlers->rx){
				handlers->rx(dev, msg->data, msg->header.len, msg->header.type, msg->header.id, msg->header.addr);
			}
		}

		k_free(msg->data);
		k_free(msg);
	}
}

static void uart_tx_thread_fn(void *arg1, void *arg2, void *arg3)
{
	LOG_DBG("Starting UART tx thread");
	int err;
	uint8_t *msg;
	const struct device *dev = (const struct device *)arg1;
	size_t size;
	while (true)
	{
		msg = k_fifo_get(&tx_fifo, K_FOREVER);
		size = *msg + sizeof(struct uart_msg_header);

		err = uart_tx(dev, msg, size, SYS_FOREVER_US);
		if (err){
			LOG_ERR("Failed to send data: Error %d", err);
		}

		err = k_sem_take(&uart_tx_sem, K_FOREVER);
		if (err){
			LOG_ERR("Failed to take semaphore: Error %d", err);
		}
	}
}