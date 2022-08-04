/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/mesh.h>
#include "bluetooth/mesh/vnd/id_srv.h"
#include "mesh/net.h"


#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(id_srv);

uint8_t *id = NULL;

static int bt_mesh_id_srv_update_handler(struct bt_mesh_model *model)
{
	if (id == NULL) {
		return -ENODATA;
	}

	bt_mesh_model_msg_init(model->pub->msg, BT_MESH_ID_OP_STATUS);
	net_buf_simple_add_mem(model->pub->msg, id, CONFIG_BT_MESH_ID_LEN);

    return 0;
}

static int bt_mesh_id_srv_identify(struct bt_mesh_model *model)
{
	static uint8_t retries = 0;
	if (retries >= 5) {
		LOG_WRN("Id server was not able to get an id");
	}

	struct bt_mesh_id_srv *srv = model->user_data;

	if (srv->handlers->identify) {
		id = srv->handlers->identify(srv);
		if (id == NULL) 
		{
			LOG_WRN("No ID, retrying");
			retries++;
			k_work_schedule(&srv->id_work, K_MSEC(50));
			return 0;
		}
	}
	LOG_INF("ID registered");

	return 0;
}

static void id_register_work(struct k_work *work)
{
	// int err;
	struct bt_mesh_id_srv *id_srv = 
		CONTAINER_OF(work, struct bt_mesh_id_srv, id_work);

	bt_mesh_id_srv_identify(id_srv->model);
}

static int bt_mesh_id_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_id_srv *srv = model->user_data;

	srv->model = model;
	srv->pub.msg = &srv->pub_msg;
	srv->pub.update = bt_mesh_id_srv_update_handler;


	net_buf_simple_init_with_data(&srv->pub_msg, srv->buf,
				      sizeof(srv->buf));

	k_work_init_delayable(&srv->id_work, id_register_work);
	LOG_INF("init id model");

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_id_srv_cb = {
	.init = bt_mesh_id_srv_init,
	.start = bt_mesh_id_srv_identify,
};
