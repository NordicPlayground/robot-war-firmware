/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/mesh.h>
#include "bluetooth/mesh/vnd/robot_cli.h"
#include "mesh/net.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(robot_cli);


int bt_mesh_robot_cli_movement_set(struct bt_mesh_robot_cli *cli,
					   	struct bt_mesh_msg_ctx *ctx,
						struct bt_mesh_movement_set set)
{
	return bt_mesh_movement_cli_movement_set(&cli->movement, ctx, set);
}

int bt_mesh_robot_cli_ready_set(struct bt_mesh_robot_cli *cli,
					   struct bt_mesh_msg_ctx *ctx)
{
	return bt_mesh_movement_cli_ready_set(&cli->movement, ctx);
}

static void handle_ack(struct bt_mesh_movement_cli *cli, struct bt_mesh_msg_ctx *ctx) 
{
	struct bt_mesh_robot_cli *robot_cli = 
		CONTAINER_OF(cli, struct bt_mesh_robot_cli, movement);
	if (robot_cli->handlers->movement_configured) {
		robot_cli->handlers->movement_configured(robot_cli, ctx);
	}
}

static const struct bt_mesh_movement_cli_handlers movement_cb = {
	.ack = handle_ack,
};

static void handle_report(struct bt_mesh_telemetry_cli *cli, 
						struct bt_mesh_msg_ctx *ctx, 
						struct bt_mesh_telemetry_report telemetry) 
{
	struct bt_mesh_robot_cli *robot_cli = 
		CONTAINER_OF(cli, struct bt_mesh_robot_cli, telemetry);
	if (robot_cli->handlers->telemetry_reported) {
		robot_cli->handlers->telemetry_reported(robot_cli, telemetry, ctx);
	}
}

static const struct bt_mesh_telemetry_cli_handlers telemetry_cb = {
	.report = handle_report,
};

static void handle_id(struct bt_mesh_id_cli *cli, struct bt_mesh_id_status id, struct bt_mesh_msg_ctx *ctx) 
{
	struct bt_mesh_robot_cli *robot_cli = 
		CONTAINER_OF(cli, struct bt_mesh_robot_cli, id);
	if (robot_cli->handlers->id) {
		robot_cli->handlers->id(robot_cli, id, ctx);
	}
}

static const struct bt_mesh_id_cli_handlers id_cb = {
	.id = handle_id,
};

static int bt_mesh_robot_cli_init(struct bt_mesh_model *model)
{
	int err;
	struct bt_mesh_robot_cli *cli = model->user_data;

	cli->model = model;

	net_buf_simple_init_with_data(&cli->pub_msg, cli->buf,
				      sizeof(cli->buf));

	cli->pub.msg = &cli->pub_msg;
	cli->id.handlers = &id_cb;
	cli->movement.handlers = &movement_cb;
	cli->telemetry.handlers = &telemetry_cb;

	err = bt_mesh_model_extend(model, cli->movement.model);
	if (err) {
		return err;
	}

	err = bt_mesh_model_extend(model, cli->telemetry.model);
	if (err) {
		return err;
	}

	return bt_mesh_model_extend(model, cli->id.model);
}

const struct bt_mesh_model_cb _bt_mesh_robot_cli_cb = {
	.init = bt_mesh_robot_cli_init,
};
