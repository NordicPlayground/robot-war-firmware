/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <app_event_manager.h>
#include <zephyr/logging/log.h>

#define MODULE main

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

void main(void)
{
	if (app_event_manager_init()) {
		LOG_ERR("Application Event Manager not initialized");
	} 
}
