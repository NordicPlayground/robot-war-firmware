/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_MESH_ID_SRV_H__
#define BT_MESH_ID_SRV_H__


#include <bluetooth/mesh/vnd/id.h>
#include <zephyr/bluetooth/mesh/access.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ID_SRV_MODEL_ID 0x000C


struct bt_mesh_id_srv;



/** @def BT_MESH_MODEL_ID_SRV
 *
 * @brief Bluetooth Mesh Id Server model composition data entry.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_id_srv instance.
 */
#define BT_MESH_MODEL_ID_SRV(_srv)                                          \
		BT_MESH_MODEL_VND_CB(CONFIG_BT_COMPANY_ID,      \
			ID_SRV_MODEL_ID,                      	\
			BT_MESH_MODEL_NO_OPS, &(_srv)->pub,       \
			BT_MESH_MODEL_USER_DATA(					\
				struct bt_mesh_id_srv, _srv), 		\
			&_bt_mesh_id_srv_cb)

/** Bluetooth Mesh id server model handlers. */
struct bt_mesh_id_srv_handlers {

	/** @brief Handler for server starting. Register id to be published. 
	 *
	 * @param[in]  srv Id Server.
	 * @param[out] id Id of the server.
	 * 
	 * @retval Id status data.
	 */
	uint8_t *(*const identify)(struct bt_mesh_id_srv *srv);
};

/**
 * Light CTL Server instance. Should be initialized with
 * @ref BT_MESH_LIGHT_CTL_SRV_INIT.
 */
struct bt_mesh_id_srv {
    /** Application handler functions. */
	const struct bt_mesh_id_srv_handlers * handlers;
	/** Model entry. */
	struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_msg;
	/* Publication data */
	uint8_t buf[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_ID_OP_STATUS, CONFIG_BT_MESH_ID_LEN)];
	/* Id registration work */
	struct k_work_delayable id_work;
};

extern const struct bt_mesh_model_cb _bt_mesh_id_srv_cb;

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_ID_SRV_H__ */

