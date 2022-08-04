/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

/**
 * @brief MESH Event
 * @defgroup mesh_module_event MESH Event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

// #include "bluetooth/mesh/vnd/robot_cli.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ID_CLI_MODEL_ID 0x000B
#define MOVEMENT_CLI_MODEL_ID 0x000D

#define BT_MESH_MODEL_OP_3(b0, cid) ((((b0) << 16) | 0xc00000) | (cid))
#define BT_MESH_ID_OP_STATUS BT_MESH_MODEL_OP_3(0x0A, 0x0059)
#define BT_MESH_MOVEMENT_OP_MOVEMENT_SET BT_MESH_MODEL_OP_3(0x0B, 0x0059)
#define BT_MESH_MOVEMENT_OP_MOVEMENT_ACK BT_MESH_MODEL_OP_3(0x0C, 0x0059)
#define BT_MESH_MOVEMENT_OP_READY_SET BT_MESH_MODEL_OP_3(0x0D, 0x0059)

enum mesh_module_event_type {
    MESH_EVT_READY, // Mesh module is ready to use.
	MESH_EVT_ROBOT_ID, // Robot added to network.
	MESH_EVT_MOVEMENT_CONFIGURED, // Robot added to network.
};

/** Id status message parameters.  */
struct bt_mesh_id_status {
	/** static identity of device */
	uint64_t id;
};

struct mesh_module_event {
    struct app_event_header header;
    enum mesh_module_event_type type;
    uint16_t addr;
    union {
        struct bt_mesh_id_status robot_id; // MESH_EVT_ROBOT_ADDED: Data about new robot.
    } data;
};

APP_EVENT_TYPE_DECLARE(mesh_module_event);

#ifdef __cplusplus
}
#endif