/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#ifndef BT_MESH_ROBOT_H__
#define BT_MESH_ROBOT_H__

#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/model_types.h>
#include <bluetooth/mesh/vnd/id.h>
#include <bluetooth/mesh/vnd/movement.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OP_VND_ROBOT_SET BT_MESH_MODEL_OP_3(0x0E, \
				       CONFIG_BT_COMPANY_ID)
                       
#ifdef __cplusplus
}
#endif

#define BT_MESH_ROBOT_MSG_MAXLEN_STATUS 5

#endif /* BT_MESH_ROBOT_H__ */
