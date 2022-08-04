/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh.h>
#include <dk_buttons_and_leds.h>
#include <bluetooth/mesh/dk_prov.h>

#define MODULE main

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_APPLICATION_MODULE_LOG_LEVEL);

#include "uart_handler.h"
#include "model_handler.h"

static const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart1));

static void mesh_rx(uint8_t *data, uint8_t len, uint32_t type, uint16_t model_id, uint16_t addr)
{
	// LOG_INF("Received id, type: %d, len: %d, addr: %d",  type, len, addr);
	// LOG_HEXDUMP_INF(data, len, "message:");
	uart_send(uart, data, (uint32_t)len, (uint32_t)type, (uint32_t)model_id, (uint32_t)addr);
}

struct model_handlers mesh_handlers = {
	.rx = mesh_rx,
};


static void uart_rx(const struct device *dev, uint8_t *data, uint8_t len, uint32_t type, uint16_t id, uint16_t addr)
{
	LOG_INF("Received uart message, type: %x, len: %x, id: %x",  type, len, id);
	LOG_HEXDUMP_INF(data, len, "message:");
	mesh_tx(data, len, type, id, addr);

}

static void uart_tx_done(const struct device *dev, const uint8_t *data, uint8_t len)
{
	// LOG_INF("Sent uart message, len: %d", len);
	// LOG_HEXDUMP_INF(data, len, "message:");
}

struct uart_msg_handlers uart_handlers = {
	.rx = uart_rx,
	.tx_done = uart_tx_done,
};

static int setup_mesh()
{
	int err;
	err = bt_enable(NULL);
	if (err)
	{
		LOG_ERR("Failed to initialize bluetooth: Error %d", err);
		return err;
	}
	LOG_DBG("Bluetooth initialized");
	err = bt_mesh_init(bt_mesh_dk_prov_init(), model_handler_init(&mesh_handlers));
	if (err)
	{
		LOG_ERR("Failed to initialize mesh: Error %d", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_SETTINGS))
	{
		err = settings_load();
		if (err)
		{
			LOG_ERR("Failed to load settings: Error %d", err);
		}
	}

	err = bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
	if (err == -EALREADY)
	{
		LOG_DBG("Device already provisioned");
		LOG_DBG("Mesh initialized");
	}

	return 0;
}

void main(void)
{
	int err = 0;
	err = setup_mesh();
	if (err)
	{
		LOG_ERR("Failed to setup mesh: Error %d", err);
		return;
	}

	err = init_uart(uart, &uart_handlers);
	if (err)
	{
		LOG_ERR("Could not initialize UART: Error %d", err);
		return;
	}
}
