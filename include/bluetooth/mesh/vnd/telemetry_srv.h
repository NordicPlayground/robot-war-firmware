/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef TELEMETRY_SRV_H__
#define TELEMETRY_SRV_H__


#include <bluetooth/mesh/vnd/telemetry.h>
#include <zephyr/bluetooth/mesh/access.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TELEMETRY_SRV_MODEL_ID 0x0012

struct bt_mesh_telemetry_srv;

/** @def BT_MESH_MODEL_TELEMETRY_SRV
 *
 * @brief Bluetooth Mesh Telemetry Server model composition data entry.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_telemetry_srv instance.
 */
#define BT_MESH_MODEL_TELEMETRY_SRV(_srv)			 \
		BT_MESH_MODEL_VND_CB(CONFIG_BT_COMPANY_ID,   \
			TELEMETRY_SRV_MODEL_ID,                  \
			BT_MESH_MODEL_NO_OPS, &(_srv)->pub,       	 \
			BT_MESH_MODEL_USER_DATA(				 \
				struct bt_mesh_telemetry_srv, _srv), \
			&_bt_mesh_telemetry_srv_cb)
		
/**
 * Light CTL Server instance. Should be initialized with
 * @ref BT_MESH_LIGHT_CTL_SRV_INIT.
 */
struct bt_mesh_telemetry_srv {
    /** Application handler functions. */
	const struct bt_mesh_telemetry_srv_handlers * handlers;
	/** Model entry. */
	struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_msg;
	/* Publication data */
	uint8_t buf[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_TELEMETRY_OP_TELEMETRY_REPORT, 
		BT_MESH_LEN_EXACT(1))];
	/** Transaction ID tracker for the set messages. */
	struct bt_mesh_tid_ctx prev_transaction;
};

int bt_mesh_telemetry_report(struct bt_mesh_telemetry_srv *srv,
					   		struct bt_mesh_telemetry_report telemetry);

extern const struct bt_mesh_model_cb _bt_mesh_telemetry_srv_cb;

#ifdef __cplusplus
}
#endif

#endif /* TELEMETRY_SRV_H__ */

