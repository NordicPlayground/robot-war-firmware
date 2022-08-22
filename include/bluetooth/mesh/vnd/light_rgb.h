/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#ifndef BT_MESH_LIGHT_RGB_H__
#define BT_MESH_LIGHT_RGB_H__

#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Light RGB set message parameters.  */
struct bt_mesh_light_rgb_set {
	/** Speed the device should move at */
	uint16_t blink_time;
	/** Amount of time the device should be moving */
	uint8_t red;
	/** Angle device should turn before moving */
	uint8_t green;
	/** Speed the device should move at */
	uint8_t blue;
};

#define BT_MESH_LIGHT_RGB_OP_RGB_SET BT_MESH_MODEL_OP_3(0x10, \
				       CONFIG_BT_COMPANY_ID)


#ifdef __cplusplus
}
#endif

#define BT_MESH_LIGHT_RGB_MSG_LEN_SET 10

#endif /* BT_MESH_LIGHT_RGB_H__ */

