/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_MESH_ROBOT_CLI_H__
#define BT_MESH_ROBOT_CLI_H__

#include <bluetooth/mesh/vnd/robot.h>
#include <bluetooth/mesh/vnd/id_cli.h>
#include <bluetooth/mesh/vnd/movement_cli.h>
#include <bluetooth/mesh/vnd/telemetry_cli.h>
#include <zephyr/bluetooth/mesh/access.h>

#ifdef __cplusplus
extern "C" {
#endif


#define ROBOT_CLI_MODEL_ID 0x000F

struct bt_mesh_robot_cli;

/** @def BT_MESH_ROBOT_CLI_INIT
 *
 * @brief Initialization parameters for the @ref bt_mesh_robot_cli.
 *
 * @param[in] _handlers Optional message handler structure.
 * @sa bt_mesh_robot_cli_handlers.
 */
#define BT_MESH_ROBOT_CLI_INIT(_handlers)                                       \
	{                                                                      \
		.handlers = _handlers,                                         \
	}

/** @def BT_MESH_MODEL_ROBOT_CLI
 *
 * @brief Bluetooth Mesh Robot Server model composition data entry.
 *
 * @param[in] _cli Pointer to a @ref bt_mesh_robot_cli instance.
 */
#define BT_MESH_MODEL_ROBOT_CLI(_cli)      \
		BT_MESH_MODEL_ID_CLI(&(_cli)->id), \
		BT_MESH_MODEL_MOVEMENT_CLI(&(_cli)->movement), \
		BT_MESH_MODEL_TELEMETRY_CLI(&(_cli)->telemetry), \
		BT_MESH_MODEL_VND_CB(CONFIG_BT_COMPANY_ID,      \
			ROBOT_CLI_MODEL_ID,                      	\
			BT_MESH_MODEL_NO_OPS, &(_cli)->pub,       \
			BT_MESH_MODEL_USER_DATA(					\
				struct bt_mesh_robot_cli, _cli), 		\
			&_bt_mesh_robot_cli_cb)

/** Bluetooth Mesh robot server model handlers. */
struct bt_mesh_robot_cli_handlers {

	/** @brief Handler for robots identifying themselves.
	 *
	 * @param[in] cli Robot Server.
	 * @param[in] id Id of robot client.
	 * @param[in] addr Address of robot client.
	 */
	void (*const id)
		(struct bt_mesh_robot_cli *cli, struct bt_mesh_id_status id, struct bt_mesh_msg_ctx *ctx);

	/** @brief Handler for robot acknowledgement of movement configuration.
	 *
	 * @param[in] cli Robot Server.
	 * @param[in] addr Address of robot client.
	 */
	void (*const movement_configured)(struct bt_mesh_robot_cli *cli, struct bt_mesh_msg_ctx *ctx);

	/** @brief Handler for robot acknowledgement of movement configuration.
	 *
	 * @param[in] cli Robot Server.
	 * @param[in] addr Address of robot client.
	 */
	void (*const telemetry_reported)
		(struct bt_mesh_robot_cli *cli, struct bt_mesh_telemetry_report report, struct bt_mesh_msg_ctx *ctx);
};

/**
 * Light CTL Server instance. Should be initialized with
 * @ref BT_MESH_LIGHT_CTL_CLI_INIT.
 */
struct bt_mesh_robot_cli {
	/** Light Level Server instance. */
	struct bt_mesh_id_cli id;
	/** Light Level Server instance. */
	struct bt_mesh_movement_cli movement;
	/** Light Level Server instance. */
	struct bt_mesh_telemetry_cli telemetry;
    /** Application handler functions. */
	const struct bt_mesh_robot_cli_handlers * handlers;
	/** Model entry. */
	struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_msg;
	/* Publication data */
	uint8_t buf[BT_MESH_MODEL_BUF_LEN(
		OP_VND_ROBOT_SET, CONFIG_BT_MESH_ID_LEN)];
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
int bt_mesh_robot_cli_movement_set(struct bt_mesh_robot_cli *cli,
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
int bt_mesh_robot_cli_ready_set(struct bt_mesh_robot_cli *cli,
					   struct bt_mesh_msg_ctx *ctx);

extern const struct bt_mesh_model_cb _bt_mesh_robot_cli_cb;

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_ROBOT_CLI_H__ */