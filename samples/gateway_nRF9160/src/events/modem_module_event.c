/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "modem_module_event.h"


static void profile_modem_module_event(struct log_event_buf *buf,
			      const struct app_event_header *aeh)
{
}

APP_EVENT_INFO_DEFINE(modem_module_event,
		  ENCODE(),
		  ENCODE(),
		  profile_modem_module_event);

static char *type_to_str(enum modem_module_event_type type)
{
    switch (type)
    {
    case MODEM_EVT_LTE_CONNECTING:
        return "MODEM_EVT_LTE_CONNECTING";
    case MODEM_EVT_LTE_CONNECTED:
        return "MODEM_EVT_LTE_CONNECTED"; 
    case MODEM_EVT_LTE_DISCONNECTED:
        return "MODEM_EVT_LTE_DISCONNECTED";
    case MODEM_EVT_ERROR:
        return "MODEM_EVT_ERROR";
    default:
        return "UNKNOWN";
    }
}

static void log_modem_module_evt(const struct app_event_header *evt)
{

    struct modem_module_event *modem_module_evt = cast_modem_module_event(evt);

    APP_EVENT_MANAGER_LOG(evt, "Type: %s", type_to_str(modem_module_evt->type));
}

APP_EVENT_TYPE_DEFINE(modem_module_event,
		  log_modem_module_evt,
		  &modem_module_event_info,
		  APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));
