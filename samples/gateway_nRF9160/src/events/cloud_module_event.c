/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "cloud_module_event.h"


static void profile_cloud_module_event(struct log_event_buf *buf,
			      const struct app_event_header *aeh)
{
}

static char *type_to_str(enum cloud_module_event_type type)
{
    switch (type)
    {
    case CLOUD_EVT_CONNECTED:
        return "CLOUD_EVT_CONNECTED";
    case CLOUD_EVT_CONNECTING:
        return "CLOUD_EVT_CONNECTING"; 
    case CLOUD_EVT_DISCONNECTED:
        return "CLOUD_EVT_DISCONNECTED";
    case CLOUD_EVT_CONNECTION_TIMEOUT:
        return "CLOUD_EVT_CONNECTION_TIMEOUT";
    case CLOUD_EVT_SEND_QOS:
        return "CLOUD_EVT_SEND_QOS"; 
    case CLOUD_EVT_SEND_QOS_CLEAR:
        return "CLOUD_EVT_SEND_QOS_CLEAR";
    case CLOUD_EVT_UPDATE_DELTA:
        return "CLOUD_EVT_UPDATE_DELTA";
    case CLOUD_EVT_ERROR:
        return "CLOUD_EVT_ERROR";
    default:
        return "UNKNOWN";
    }
}

static void log_cloud_module_evt(const struct app_event_header *evt)
{

    struct cloud_module_event *cloud_module_evt = cast_cloud_module_event(evt);

    APP_EVENT_MANAGER_LOG(evt, "Type: %s", type_to_str(cloud_module_evt->type));
}

APP_EVENT_INFO_DEFINE(cloud_module_event,
		  ENCODE(),
		  ENCODE(),
		  profile_cloud_module_event);

APP_EVENT_TYPE_DEFINE(cloud_module_event,
		  log_cloud_module_evt,
		  &cloud_module_event_info,
		  APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));
