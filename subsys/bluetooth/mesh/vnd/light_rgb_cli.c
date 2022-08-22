/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/mesh.h>
#include "bluetooth/mesh/vnd/light_rgb_cli.h"
// #include "model_utils.h"
#include "mesh/net.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(light_rgb_cli);

int bt_mesh_light_rgb_cli_rgb_set(struct bt_mesh_light_rgb_cli *cli,
					   struct bt_mesh_msg_ctx *ctx,
					   const struct bt_mesh_light_rgb_set set)
{
	if (!cli || !ctx) {
		return -EINVAL;
	}

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_RGB_OP_RGB_SET, sizeof(struct bt_mesh_light_rgb_set));
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_RGB_OP_RGB_SET);
	net_buf_simple_add_be16(&buf, set.blink_time);
	net_buf_simple_add_u8(&buf, set.red);
	net_buf_simple_add_u8(&buf, set.green);
	net_buf_simple_add_u8(&buf, set.blue);

	LOG_INF("sending packet over mesh! %d", buf.len);
	LOG_HEXDUMP_INF(buf.data, buf.len, "packet:");

	return bt_mesh_model_send(cli->model, ctx, &buf, NULL, NULL);
}

static int bt_mesh_light_rgb_cli_init(struct bt_mesh_model *model)
{
	struct bt_mesh_light_rgb_cli *cli = model->user_data;

	cli->model = model;

	net_buf_simple_init_with_data(&cli->pub_msg, cli->buf,
				      sizeof(cli->buf));

	cli->pub.msg = &cli->pub_msg;

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_light_rgb_cli_cb = {
	.init = bt_mesh_light_rgb_cli_init,
};