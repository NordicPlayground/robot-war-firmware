/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_MESH_ID_CLI_H__
#define BT_MESH_ID_CLI_H__

#include <bluetooth/mesh/vnd/id.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ID_CLI_MODEL_ID 0x000B

struct bt_mesh_id_cli;

/** @def BT_MESH_ID_CLI_INIT
 *
 * @brief Initialization parameters for the @ref bt_mesh_id_cli.
 *
 * @param[in] _handlers Optional message handler structure.
 * @sa bt_mesh_id_cli_handlers.
 */
#define BT_MESH_ID_CLI_INIT(_handlers)                                       \
	{                                                                      \
		.handlers = _handlers,                                         \
	}

/** @def BT_MESH_MODEL_ID_CLI
 *
 * @brief Bluetooth Mesh Id Server model composition data entry.
 *
 * @param[in] _cli Pointer to a @ref bt_mesh_id_cli instance.
 */
#define BT_MESH_MODEL_ID_CLI(_cli)                                          \
		BT_MESH_MODEL_VND_CB(CONFIG_BT_COMPANY_ID,      \
			ID_CLI_MODEL_ID,                      	\
			_bt_mesh_id_cli_op, &(_cli)->pub,       \
			BT_MESH_MODEL_USER_DATA(					\
				struct bt_mesh_id_cli, _cli), 		\
			&_bt_mesh_id_cli_cb)

/** Bluetooth Mesh id server model handlers. */
struct bt_mesh_id_cli_handlers {

	/** @brief Handler for server starting. Register id to be published. 
	 *
	 * @param[in] cli Id Server.
	 * @param[in] id Id of client.
	 * @param[in] addr address of client.
	 */
	void (*const id)(struct bt_mesh_id_cli *cli, struct bt_mesh_id_status id, struct bt_mesh_msg_ctx *ctx);
};

/**
 * Light CTL Server instance. Should be initialized with
 * @ref BT_MESH_LIGHT_CTL_CLI_INIT.
 */
struct bt_mesh_id_cli {
    /** Application handler functions. */
	const struct bt_mesh_id_cli_handlers * handlers;
	/** Model entry. */
	struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_msg;
	/* Publication data */
	uint8_t buf[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_ID_OP_STATUS, CONFIG_BT_MESH_ID_LEN)];
};


extern const struct bt_mesh_model_op _bt_mesh_id_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_id_cli_cb;

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_ID_CLI_H__ */