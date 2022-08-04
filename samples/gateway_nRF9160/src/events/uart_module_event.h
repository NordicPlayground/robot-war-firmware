/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

/**
 * @brief UART Event
 * @defgroup uart_module_event UART Event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

enum uart_module_event_type {
    UART_EVT_READY,
	UART_EVT_RX,
    UART_EVT_TX,
	UART_EVT_ERROR, 
};
struct uart_packet_header {
    uint32_t len;
    uint32_t type;
    uint32_t id;
	uint32_t addr;
};

struct uart_packet {
    struct uart_packet_header header;
    void *data;
};

struct uart_module_event {
    struct app_event_header header;
    enum uart_module_event_type type;
    union {
        struct uart_packet rx; 
		struct uart_packet tx; 
        int err;
    } data;
};

APP_EVENT_TYPE_DECLARE(uart_module_event);

#ifdef __cplusplus
}
#endif