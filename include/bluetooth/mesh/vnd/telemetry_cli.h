/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef TELEMETRY_CLI_H__
#define TELEMETRY_CLI_H__


#include <bluetooth/mesh/vnd/telemetry.h>
#include <zephyr/bluetooth/mesh/access.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TELEMETRY_CLI_MODEL_ID 0x0011

struct bt_mesh_telemetry_cli;

/** Bluetooth Mesh Telemetry Server model handlers. */
struct bt_mesh_telemetry_cli_handlers {
	/** @brief Handler for an report message. 
	 *
	 * @param[in] cli Telemetry Server that received the report message.
	 */
	void (*const report)
		(struct bt_mesh_telemetry_cli *cli, struct bt_mesh_msg_ctx *ctx, struct bt_mesh_telemetry_report telemetry);
};

/** @def BT_MESH_MODEL_TELEMETRY_CLI
 *
 * @brief Bluetooth Mesh Telemetry Server model composition data entry.
 *
 * @param[in] _cli Pointer to a @ref bt_mesh_telemetry_cli instance.
 */
#define BT_MESH_MODEL_TELEMETRY_CLI(_cli)			\
		BT_MESH_MODEL_VND_CB(CONFIG_BT_COMPANY_ID,      \
			TELEMETRY_CLI_MODEL_ID,                      \
			_bt_mesh_telemetry_cli_op, NULL,       		\
			BT_MESH_MODEL_USER_DATA(					\
				struct bt_mesh_telemetry_cli, _cli), \
			&_bt_mesh_telemetry_cli_cb)

/**
 * Light CTL Server instance. Should be initialized with
 * @ref BT_MESH_LIGHT_CTL_CLI_INIT.
 */
struct bt_mesh_telemetry_cli {
    /** Application handler functions. */
	const struct bt_mesh_telemetry_cli_handlers * handlers;
	/** Model entry. */
	struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_msg;
	/* Publication data */
	uint8_t buf[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_TELEMETRY_OP_TELEMETRY_REPORT, 
		BT_MESH_LEN_MIN(sizeof(struct bt_mesh_telemetry_report)))];
	/** Transaction ID tracker for the set messages. */
	struct bt_mesh_tid_ctx prev_transaction;
};

extern const struct bt_mesh_model_op _bt_mesh_telemetry_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_telemetry_cli_cb;

#ifdef __cplusplus
}
#endif

#endif /* MOVEMENT_CLI_H__ */

