/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LIGHT_RGB_SRV_H__
#define LIGHT_RGB_SRV_H__


#include <bluetooth/mesh/vnd/light_rgb.h>
#include <zephyr/bluetooth/mesh/access.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LIGHT_RGB_SRV_MODEL_ID 0x0014

struct bt_mesh_light_rgb_srv;

/** Bluetooth Mesh Light RGB Server model handlers. */
struct bt_mesh_light_rgb_srv_handlers {
	/** @brief Handler for a set rgb message. 
	 *
	 * @param[in] srv Light RGB Server that received the set message.
	 * @param[in] rgb The message containing the rgb data.
	 */
	void (*const set)(struct bt_mesh_light_rgb_srv *srv, 
			struct bt_mesh_light_rgb_set *rgb);
};

/** @def BT_MESH_MODEL_LIGHT_RGB_SRV
 *
 * @brief Bluetooth Mesh Light RGB Server model composition data entry.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_light_rgb_srv instance.
 */
#define BT_MESH_MODEL_LIGHT_RGB_SRV(_srv)			\
		BT_MESH_MODEL_VND_CB(CONFIG_BT_COMPANY_ID,      \
			LIGHT_RGB_SRV_MODEL_ID,                      \
			_bt_mesh_light_rgb_srv_op, NULL,       		\
			BT_MESH_MODEL_USER_DATA(					\
				struct bt_mesh_light_rgb_srv, _srv), \
			&_bt_mesh_light_rgb_srv_cb)
		
/**
 * Light CTL Server instance. Should be initialized with
 * @ref BT_MESH_LIGHT_CTL_SRV_INIT.
 */
struct bt_mesh_light_rgb_srv {
    /** Application handler functions. */
	const struct bt_mesh_light_rgb_srv_handlers * handlers;
	/** Model entry. */
	struct bt_mesh_model *model;
	/** Transaction ID tracker for the set messages. */
	struct bt_mesh_tid_ctx prev_transaction;
};

extern const struct bt_mesh_model_op _bt_mesh_light_rgb_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_light_rgb_srv_cb;

#ifdef __cplusplus
}
#endif

#endif /* LIGHT_RGB_SRV_H__ */

