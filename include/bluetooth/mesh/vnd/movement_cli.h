/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOVEMENT_CLI_H__
#define MOVEMENT_CLI_H__


#include <bluetooth/mesh/vnd/movement.h>
#include <zephyr/bluetooth/mesh/access.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MOVEMENT_CLI_MODEL_ID 0x000D

struct bt_mesh_movement_cli;

/** Bluetooth Mesh Movement Server model handlers. */
struct bt_mesh_movement_cli_handlers {
	/** @brief Handler for an ack message. 
	 *
	 * @param[in] cli Movement Server that received the set message.
	 */
	void (*const ack)
		(struct bt_mesh_movement_cli *cli, struct bt_mesh_msg_ctx *ctx);
};

/** @def BT_MESH_MODEL_MOVEMENT_CLI
 *
 * @brief Bluetooth Mesh Movement Server model composition data entry.
 *
 * @param[in] _cli Pointer to a @ref bt_mesh_movement_cli instance.
 */
#define BT_MESH_MODEL_MOVEMENT_CLI(_cli)			\
		BT_MESH_MODEL_VND_CB(CONFIG_BT_COMPANY_ID,      \
			MOVEMENT_CLI_MODEL_ID,                      \
			_bt_mesh_movement_cli_op, NULL,       		\
			BT_MESH_MODEL_USER_DATA(					\
				struct bt_mesh_movement_cli, _cli), \
			&_bt_mesh_movement_cli_cb)
		
/**
 * Light CTL Server instance. Should be initialized with
 * @ref BT_MESH_LIGHT_CTL_CLI_INIT.
 */
struct bt_mesh_movement_cli {
    /** Application handler functions. */
	const struct bt_mesh_movement_cli_handlers * handlers;
	/** Model entry. */
	struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_msg;
	/* Publication data */
	uint8_t buf[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_MOVEMENT_OP_MOVEMENT_SET, 
		BT_MESH_LEN_MIN(sizeof(struct bt_mesh_movement_set)))];
	/** Transaction ID tracker for the set messages. */
	struct bt_mesh_tid_ctx prev_transaction;
};

/** @brief Set the movement configuration on a Movement Server.
 *
 *  The movement configuration determines the actions the movement
 *  server will actuate on the next ready event. 
 *
 *  @param[in]  cli Client model to send on.
 *  @param[in]  ctx Message context, or NULL to use the configured publish
 *                  parameters.
 *  @param[in]  set Set parameters.
 *
 *  @retval 0              Successfully sent the message.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 */
int bt_mesh_movement_cli_movement_set(struct bt_mesh_movement_cli *cli,
					   struct bt_mesh_msg_ctx *ctx,
					   struct bt_mesh_movement_set set);

/** @brief Notify Movement Server that it is clear to execute movement configuration.
 *
 *
 *  @param[in]  cli Client model to send on.
 *  @param[in]  ctx Message context, or NULL to use the configured publish
 *                  parameters.
 *
 *  @retval 0              Successfully sent the message.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 */
int bt_mesh_movement_cli_ready_set(struct bt_mesh_movement_cli *cli,
					   struct bt_mesh_msg_ctx *ctx);

extern const struct bt_mesh_model_op _bt_mesh_movement_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_movement_cli_cb;

#ifdef __cplusplus
}
#endif

#endif /* MOVEMENT_CLI_H__ */

