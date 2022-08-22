/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "ui_module_event.h"


static void profile_ui_module_event(struct log_event_buf *buf,
			      const struct app_event_header *aeh)
{
}

APP_EVENT_INFO_DEFINE(ui_module_event,
		  ENCODE(),
		  ENCODE(),
		  profile_ui_module_event);

static char *type_to_str(enum ui_module_event_type type)
{
    switch (type)
    {
    case UI_EVT_BUTTON:
        return "UI_EVT_BUTTON";
    case UI_EVT_LED:
        return "UI_EVT_LED"; 
    case UI_EVT_ERROR:
        return "UI_EVT_ERROR";
    default:
        return "UNKNOWN";
    }
}

static void log_ui_module_evt(const struct app_event_header *evt)
{

    struct ui_module_event *ui_module_evt = cast_ui_module_event(evt);

    APP_EVENT_MANAGER_LOG(evt, "Type: %s", type_to_str(ui_module_evt->type));
}

APP_EVENT_TYPE_DEFINE(ui_module_event,
		  log_ui_module_evt,
		  &ui_module_event_info,
		  APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));
