/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "robot_module_event.h"


static void profile_robot_module_event(struct log_event_buf *buf,
			      const struct app_event_header *aeh)
{
}

APP_EVENT_INFO_DEFINE(robot_module_event,
		  ENCODE(),
		  ENCODE(),
		  profile_robot_module_event);

static char *type_to_str(enum robot_module_event_type type)
{
    switch (type)
    {
    case ROBOT_EVT_REPORT:
        return "ROBOT_EVT_REPORT";
    // case ROBOT_EVT_CLEAR_ALL:
    //     return "ROBOT_EVT_CLEAR_ALL"; 
    case ROBOT_EVT_MOVEMENT_CONFIGURE:
        return "ROBOT_EVT_MOVEMENT_CONFIGURE";
    case ROBOT_EVT_LED_CONFIGURE:
        return "ROBOT_EVT_LED_CONFIGURE";
    case ROBOT_EVT_ERROR:
        return "ROBOT_EVT_ERROR";
    case ROBOT_EVT_CLEAR_TO_MOVE:
        return "ROBOT_EVT_CLEAR_TO_MOVE";
    default:
        return "UNKNOWN";
    }
}

static void log_robot_module_evt(const struct app_event_header *evt)
{

    struct robot_module_event *robot_module_evt = cast_robot_module_event(evt);

    APP_EVENT_MANAGER_LOG(evt, "Type: %s", type_to_str(robot_module_evt->type));
}

APP_EVENT_TYPE_DEFINE(robot_module_event,
		  log_robot_module_evt,
		  &robot_module_event_info,
		  APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));
