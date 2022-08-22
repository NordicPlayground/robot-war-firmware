/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_MESH_TELEMETRY_H__
#define BT_MESH_TELEMETRY_H__

#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Telemetry set message parameters.  */
struct bt_mesh_telemetry_report {
	uint8_t revolutions;
};

#define BT_MESH_TELEMETRY_OP_TELEMETRY_REPORT BT_MESH_MODEL_OP_3(0x0F, \
				       CONFIG_BT_COMPANY_ID)

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_TELEMETRY_H__ */

