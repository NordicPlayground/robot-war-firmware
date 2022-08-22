/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/mesh.h>
#include "bluetooth/mesh/vnd/light_rgb_srv.h"
#include "mesh/net.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(light_rgb_srv);

struct bt_mesh_light_rgb_set extract_rgb(struct net_buf_simple *buf)
{
	struct bt_mesh_light_rgb_set rgb;
    rgb.blink_time = net_buf_simple_pull_be16(buf);
    rgb.red = net_buf_simple_pull_u8(buf);
    rgb.green = net_buf_simple_pull_u8(buf);
    rgb.blue = net_buf_simple_pull_u8(buf);
	return rgb;
}

static int handle_message_light_rgb_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	LOG_INF("received message");
	struct bt_mesh_light_rgb_srv *srv = model->user_data;
	struct bt_mesh_light_rgb_set rgb;

	rgb = extract_rgb(buf);

	if (srv->handlers->set) {
		srv->handlers->set(srv, &rgb);
	}

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_light_rgb_srv_op[] = {
	{
		BT_MESH_LIGHT_RGB_OP_RGB_SET, BT_MESH_LEN_EXACT(5),
		handle_message_light_rgb_set
	},
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_light_rgb_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_light_rgb_srv *srv = model->user_data;

	srv->model = model;

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_light_rgb_srv_cb = {
	.init = bt_mesh_light_rgb_srv_init,
};