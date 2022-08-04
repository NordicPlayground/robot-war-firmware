/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/mesh.h>
#include "bluetooth/mesh/vnd/id_cli.h"
#include "mesh/net.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(id_cli);

struct bt_mesh_id_status extract_identity(struct net_buf_simple *buf)
{
	struct bt_mesh_id_status status;

	for (size_t i = 0; i < CONFIG_BT_MESH_ID_LEN; i++)
	{
		status.id[i] = net_buf_simple_pull_u8(buf);
	}
	return status;
}

static int handle_message_id_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct bt_mesh_id_cli *cli = model->user_data;
	struct bt_mesh_id_status status;

	status = extract_identity(buf);
	if(cli->handlers->id) {
		cli->handlers->id(cli, status, ctx);
	}
	return 0;
}

const struct bt_mesh_model_op _bt_mesh_id_cli_op[] = {
	{
		BT_MESH_ID_OP_STATUS, BT_MESH_LEN_EXACT(CONFIG_BT_MESH_ID_LEN), handle_message_id_status
	},
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_id_cli_init(struct bt_mesh_model *model)
{
	
	struct bt_mesh_id_cli *cli = model->user_data;

	cli->model = model;

	net_buf_simple_init_with_data(&cli->pub_msg, cli->buf,
				      sizeof(cli->buf));
	
	cli->pub.msg = &cli->pub_msg;
	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_id_cli_cb = {
	.init = bt_mesh_id_cli_init,
};
