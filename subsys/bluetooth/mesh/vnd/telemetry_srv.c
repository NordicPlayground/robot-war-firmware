/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/mesh.h>
#include "bluetooth/mesh/vnd/telemetry_srv.h"
#include "mesh/net.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(telemetry_srv);

int bt_mesh_telemetry_report(struct bt_mesh_telemetry_srv *srv,
					   		struct bt_mesh_telemetry_report telemetry)
{
	bt_mesh_model_msg_init(&srv->pub_msg, BT_MESH_TELEMETRY_OP_TELEMETRY_REPORT);
	net_buf_simple_add_mem(&srv->pub_msg, &telemetry.revolutions, sizeof(uint8_t));
	return bt_mesh_model_publish(srv->model);
}

static int bt_mesh_telemetry_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_telemetry_srv *srv = model->user_data;

	srv->model = model;
	srv->pub.msg = &srv->pub_msg;

	net_buf_simple_init_with_data(&srv->pub_msg, srv->buf,
				      sizeof(srv->buf));
	LOG_INF("init telemetry model");

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_telemetry_srv_cb = {
	.init = bt_mesh_telemetry_srv_init,
};