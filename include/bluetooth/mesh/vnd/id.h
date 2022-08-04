/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#ifndef BT_MESH_ID_H__
#define BT_MESH_ID_H__

#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Id status message parameters.  */
struct bt_mesh_id_status {
	/** static identity of device */
	uint8_t id[CONFIG_BT_MESH_ID_LEN];
};

#define BT_MESH_ID_OP_STATUS BT_MESH_MODEL_OP_3(0x0A, \
				       CONFIG_BT_COMPANY_ID)

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_ID_H__ */
