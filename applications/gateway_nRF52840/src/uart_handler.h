/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#ifndef UART_HANDLER_H__
#define UART_HANDLER_H__

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Uart message handlers. */
struct uart_msg_handlers {
    /** @brief Handler for receiving messages.
	 *
	 * @param[in] dev Uart device.
	 * @param[in] data Pointer to message payload.
	 * @param[in] len Length of payload data.
	 * @param[in] type Type of payload.
	 */
    void (*rx)(const struct device *dev, uint8_t *data, uint8_t len, uint32_t type, uint16_t id, uint16_t addr);
    /** @brief Handler for message tx done.
     * 
	 *
	 * @param[in] dev Uart device.
	 * @param[in] data Pointer to message payload.
	 * @param[in] len Length of payload data.
	 */
    void (*tx_done)(const struct device *dev, const uint8_t *data, uint8_t len);
};

/** @brief Send a message over the uart bridge.
 *
 * @param[in] dev Uart device.
 * @param[in] data Pointer to message payload.
 * @param[in] len Length of payload data.
 * @param[in] type Type of payload.
 */
int uart_send(const struct device *dev, uint8_t *data, uint32_t len, uint32_t type, uint32_t id, uint32_t addr);

/** @brief Initialize the uart message bridge.
 *
 * @param[in] dev Uart device.
 * @param[in] handlers handlers.
 */
int init_uart(const struct device *dev, struct uart_msg_handlers *handlers);


#ifdef __cplusplus
}
#endif

#endif /* UART_HANDLER_H__ */