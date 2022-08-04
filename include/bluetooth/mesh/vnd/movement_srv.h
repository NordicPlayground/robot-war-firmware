/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOVEMENT_SRV_H__
#define MOVEMENT_SRV_H__


#include <bluetooth/mesh/vnd/movement.h>
#include <zephyr/bluetooth/mesh/access.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MOVEMENT_SRV_MODEL_ID 0x000E

struct bt_mesh_movement_srv;



/** Bluetooth Mesh Movement Server model handlers. */
struct bt_mesh_movement_srv_handlers {
	/** @brief Handler for a set movement message. 
	 *
	 * @param[in] srv Movement Server that received the set message.
	 * @param[in] movement The message containing the movement data.
	 */
	void (*const set)(struct bt_mesh_movement_srv *srv, 
			struct bt_mesh_movement_set movement);
			
	/** @brief Handler for a ready to move message. 
	 *
	 * @param[in] srv Movement Server that received the ready message.
	 */
	void (*const ready)(struct bt_mesh_movement_srv *srv);
};

/** @def BT_MESH_MODEL_MOVEMENT_SRV
 *
 * @brief Bluetooth Mesh Movement Server model composition data entry.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_movement_srv instance.
 */
#define BT_MESH_MODEL_MOVEMENT_SRV(_srv)			\
		BT_MESH_MODEL_VND_CB(CONFIG_BT_COMPANY_ID,      \
			MOVEMENT_SRV_MODEL_ID,                      \
			_bt_mesh_movement_srv_op, NULL,       		\
			BT_MESH_MODEL_USER_DATA(					\
				struct bt_mesh_movement_srv, _srv), \
			&_bt_mesh_movement_srv_cb)
		
/**
 * Light CTL Server instance. Should be initialized with
 * @ref BT_MESH_LIGHT_CTL_SRV_INIT.
 */
struct bt_mesh_movement_srv {
    /** Application handler functions. */
	const struct bt_mesh_movement_srv_handlers * handlers;
	/** Model entry. */
	struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_msg;
	/* Publication data */
	uint8_t buf[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_MOVEMENT_OP_MOVEMENT_ACK, 
		BT_MESH_LEN_EXACT(sizeof(0)))];
	/** Transaction ID tracker for the set messages. */
	struct bt_mesh_tid_ctx prev_transaction;
};

extern const struct bt_mesh_model_op _bt_mesh_movement_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_movement_srv_cb;

#ifdef __cplusplus
}
#endif

#endif /* MOVEMENT_SRV_H__ */

