/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#ifndef MODEL_HANDLER_H__
#define MODEL_HANDLER_H__

#include <zephyr/bluetooth/mesh.h>
#include "bluetooth/mesh/vnd/robot_cli.h"
#include "bluetooth/mesh/vnd/light_rgb_cli.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Uart message handlers. */
struct model_handlers {
    /** @brief Handler for receiving messages.
	 *
	 * @param[in] data Pointer to message payload.
	 * @param[in] len Length of payload data.
	 * @param[in] type Type of payload.
	 * @param[in] add addr of client.
	 */
    void (*rx)(uint8_t *data, uint8_t len, uint32_t type, uint16_t model_id, uint16_t addr);
    /** @brief Handler for message tx done.
     * 
	 * @param[in] data Pointer to message payload.
	 * @param[in] len Length of payload data.
	 */
    void (*tx_done)(uint8_t *data, uint8_t len);
};

const struct bt_mesh_comp *model_handler_init(struct model_handlers *handlers);

int mesh_tx(uint8_t *data, uint8_t len, uint32_t type, uint16_t model_id, uint16_t addr);
int set_movement(uint16_t addr, uint32_t time, uint32_t rotation, uint8_t speed);

#ifdef __cplusplus
}
#endif

#endif /* MODEL_HANDLER_H__ */
