/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LIGHT_RGB_CLI_H__
#define LIGHT_RGB_CLI_H__


#include <bluetooth/mesh/vnd/light_rgb.h>
#include <zephyr/bluetooth/mesh/access.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LIGHT_RGB_CLI_MODEL_ID 0x0013

struct bt_mesh_light_rgb_cli;


/** @def BT_MESH_MODEL_LIGHT_RGB_CLI
 *
 * @brief Bluetooth Mesh Light RGB Server model composition data entry.
 *
 * @param[in] _cli Pointer to a @ref bt_mesh_light_rgb_cli instance.
 */
#define BT_MESH_MODEL_LIGHT_RGB_CLI(_cli)			\
		BT_MESH_MODEL_VND_CB(CONFIG_BT_COMPANY_ID,      \
			LIGHT_RGB_CLI_MODEL_ID,                      \
			BT_MESH_MODEL_NO_OPS, NULL,       		\
			BT_MESH_MODEL_USER_DATA(					\
				struct bt_mesh_light_rgb_cli, _cli), \
			&_bt_mesh_light_rgb_cli_cb)
		
/**
 * Light CTL Server instance. Should be initialized with
 * @ref BT_MESH_LIGHT_CTL_CLI_INIT.
 */
struct bt_mesh_light_rgb_cli {
	/** Model entry. */
	struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_msg;
	/* Publication data */
	uint8_t buf[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_LIGHT_RGB_OP_RGB_SET, 
		BT_MESH_LEN_MIN(sizeof(struct bt_mesh_light_rgb_set)))];
	/** Transaction ID tracker for the set messages. */
	struct bt_mesh_tid_ctx prev_transaction;
};

/** @brief Set the rgb configuration on a Light RGB Server.
 *
 *  The rgb configuration determines the actions the rgb
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
int bt_mesh_light_rgb_cli_rgb_set(struct bt_mesh_light_rgb_cli *cli,
					   struct bt_mesh_msg_ctx *ctx,
					   struct bt_mesh_light_rgb_set rgb);


extern const struct bt_mesh_model_cb _bt_mesh_light_rgb_cli_cb;

#ifdef __cplusplus
}
#endif

#endif /* LIGHT_RGB_CLI_H__ */

