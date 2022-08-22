/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/mesh.h>
#include "bluetooth/mesh/vnd/telemetry_cli.h"
#include "mesh/net.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(telemetry_cli);

static int handle_message_report(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct bt_mesh_telemetry_cli *cli = model->user_data;
	struct bt_mesh_telemetry_report telemetry;
	telemetry.revolutions = net_buf_simple_pull_u8(buf);

	if (cli->handlers->report) {
		cli->handlers->report(cli, ctx, telemetry);
	}

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_telemetry_cli_op[] = {
	{
		BT_MESH_TELEMETRY_OP_TELEMETRY_REPORT, BT_MESH_LEN_EXACT(1),
		handle_message_report
	},
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_telemetry_cli_init(struct bt_mesh_model *model)
{
	struct bt_mesh_telemetry_cli *cli = model->user_data;

	cli->model = model;

	net_buf_simple_init_with_data(&cli->pub_msg, cli->buf,
				      sizeof(cli->buf));

	cli->pub.msg = &cli->pub_msg;

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_telemetry_cli_cb = {
	.init = bt_mesh_telemetry_cli_init,
};