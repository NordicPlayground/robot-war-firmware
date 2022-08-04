/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/mesh.h>
#include "bluetooth/mesh/vnd/movement_cli.h"
// #include "model_utils.h"
#include "mesh/net.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(movement_cli);

int bt_mesh_movement_cli_movement_set(struct bt_mesh_movement_cli *cli,
					   struct bt_mesh_msg_ctx *ctx,
					   const struct bt_mesh_movement_set set)
{
	if (!cli || !ctx) {
		return -EINVAL;
	}

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_MOVEMENT_OP_MOVEMENT_SET, sizeof(struct bt_mesh_movement_set));
	bt_mesh_model_msg_init(&buf, BT_MESH_MOVEMENT_OP_MOVEMENT_SET);
	net_buf_simple_add_be32(&buf, set.time);
	net_buf_simple_add_be32(&buf, set.angle);
	net_buf_simple_add_u8(&buf, set.speed);

	LOG_INF("sending packet over mesh! %d", buf.len);
	LOG_HEXDUMP_INF(buf.data, buf.len, "packet:");

	return bt_mesh_model_send(cli->model, ctx, &buf, NULL, NULL);
}

int bt_mesh_movement_cli_ready_set(struct bt_mesh_movement_cli *cli,
					   struct bt_mesh_msg_ctx *ctx)
{	
	if (!cli || !ctx) {
		return -EINVAL;
	}
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_MOVEMENT_OP_READY_SET, 0);
	bt_mesh_model_msg_init(&buf, BT_MESH_MOVEMENT_OP_READY_SET);

	return bt_mesh_model_send(cli->model, ctx, &buf, NULL, NULL);
}

// struct bt_mesh_movement_set extract_movement(struct net_buf_simple *buf)
// {
// 	struct bt_mesh_movement_set mov_conf;
//     mov_conf.time = net_buf_simple_pull_be32(buf);
//     mov_conf.angle = net_buf_simple_pull_be32(buf);
//     mov_conf.speed = net_buf_simple_pull_u8(buf);
// 	return mov_conf;
// }

// static int handle_message_movement_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
// 			  struct net_buf_simple *buf)
// {
// 	struct bt_mesh_movement_cli *cli = model->user_data;
// 	struct bt_mesh_movement_set movement;
// 	int err;

// 	movement = extract_movement(buf);

// 	if (cli->handlers->set) {
// 		cli->handlers->set(cli, movement);
// 	}

// 	BT_MESH_MODEL_BUF_DEFINE(ack, BT_MESH_MOVEMENT_OP_MOVEMENT_ACK, sizeof(0));
//     bt_mesh_model_msg_init(&ack, BT_MESH_MOVEMENT_OP_MOVEMENT_ACK);
//     net_buf_simple_add_u8(&ack, 0);
//     err = bt_mesh_model_send(model, ctx, &ack, NULL, NULL);
//     if (err)
//     {
//         printk("Failed to send message ack (err %d)", err);
//     }

// 	return 0;
// }

static int handle_message_ack(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct bt_mesh_movement_cli *cli = model->user_data;

	if (cli->handlers->ack) {
		cli->handlers->ack(cli, ctx);
	}

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_movement_cli_op[] = {
	{
		BT_MESH_MOVEMENT_OP_MOVEMENT_ACK, BT_MESH_LEN_EXACT(0),
		handle_message_ack
	},
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_movement_cli_init(struct bt_mesh_model *model)
{
	struct bt_mesh_movement_cli *cli = model->user_data;

	cli->model = model;

	net_buf_simple_init_with_data(&cli->pub_msg, cli->buf,
				      sizeof(cli->buf));

	cli->pub.msg = &cli->pub_msg;

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_movement_cli_cb = {
	.init = bt_mesh_movement_cli_init,
};