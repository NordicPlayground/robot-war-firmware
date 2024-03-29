/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _ROBOT_MODULE_EVENT_H_
#define _ROBOT_MODULE_EVENT_H_

/**
 * @brief ROBOT Event
 * @defgroup robot_module_event ROBOT Event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>
#include <cloud/codec.h>

#ifdef __cplusplus
extern "C" {
#endif

enum robot_module_event_type {
	ROBOT_EVT_REPORT,
	ROBOT_EVT_MOVEMENT_CONFIGURE,
	ROBOT_EVT_LED_CONFIGURE,
	ROBOT_EVT_ERROR,
	ROBOT_EVT_CLEAR_TO_MOVE,
};

struct robot_module_event {
	struct app_event_header header;
	enum robot_module_event_type type;
	uint16_t addr;
	union {
		struct codec_movement *movement;
		struct codec_led *led;
		char* str;
		int err;
	} data;
};

APP_EVENT_TYPE_DECLARE(robot_module_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _ROBOT_MODULE_EVENT_H_ */
