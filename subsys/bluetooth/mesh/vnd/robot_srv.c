/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/mesh.h>
#include "bluetooth/mesh/vnd/robot_srv.h"
#include "mesh/net.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(robot_srv);

uint8_t *identity = NULL;

static struct bt_mesh_movement_set movement_config;

int bt_mesh_robot_report_telemetry(struct bt_mesh_robot_srv *srv,
					  struct bt_mesh_telemetry_report telemetry)
{
	return bt_mesh_telemetry_report(&srv->telemetry, telemetry);
}

static uint8_t * handle_identify(struct bt_mesh_id_srv *srv)
{
	struct bt_mesh_robot_srv *robot_srv = 
		CONTAINER_OF(srv, struct bt_mesh_robot_srv, id);

	if (robot_srv->handlers->identify) {
		return robot_srv->handlers->identify(robot_srv);
	}
	return NULL;
}

static const struct bt_mesh_id_srv_handlers id_cb = {
	.identify = handle_identify,
};


static void handle_movement_set(struct bt_mesh_movement_srv *srv, 
				struct bt_mesh_movement_set msg)
{
	movement_config = msg;
	LOG_INF("movement set, time: %d, rotation: %d, speed: %d", movement_config.time, movement_config.angle, movement_config.speed);
}

static void handle_movement_ready(struct bt_mesh_movement_srv *srv)
{	
	struct bt_mesh_robot_srv *robot_srv = 
		CONTAINER_OF(srv, struct bt_mesh_robot_srv, movement);

	if (robot_srv->handlers->move) {
		robot_srv->handlers->move(robot_srv, &movement_config);
	}
}

static const struct bt_mesh_movement_srv_handlers movement_cb = {
	.set = handle_movement_set,
	.ready = handle_movement_ready,
};

// static int bt_mesh_robot_srv_start(struct bt_mesh_model *model)
// {
// 	struct bt_mesh_robot_srv *srv = model->user_data;

// 	if (srv->handlers->identify) {
// 		srv->handlers->identify(srv, identity);
// 	}

// 	return 0;
// }

static int bt_mesh_robot_srv_init(struct bt_mesh_model *model)
{
	int err;
	struct bt_mesh_robot_srv *srv = model->user_data;

	srv->model = model;

	net_buf_simple_init_with_data(&srv->pub_msg, srv->buf,
				      sizeof(srv->buf));

	srv->pub.msg = &srv->pub_msg;

	srv->id.handlers = &id_cb;
	srv->movement.handlers = &movement_cb;
	LOG_INF("init robot model");
	err = bt_mesh_model_extend(model, srv->movement.model);
	if (err) {
		return err;
	}
		
	err = bt_mesh_model_extend(model, srv->telemetry.model);
	if (err) {
		return err;
	}

	return bt_mesh_model_extend(model, srv->id.model);
}

const struct bt_mesh_model_cb _bt_mesh_robot_srv_cb = {
	.init = bt_mesh_robot_srv_init,
};
