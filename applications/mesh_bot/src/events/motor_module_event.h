#pragma once

#include <app_event_manager.h>

typedef enum {
    MOTOR_EVT_MOVEMENT_START,
    MOTOR_EVT_MOVEMENT_DONE,
    MOTOR_EVT_MOVEMENT_REPORT,
} motor_module_event_type;

struct movement_report {
    uint8_t revolutions;
};

struct motor_module_event {
    struct app_event_header header;
    motor_module_event_type type;
    union {
        struct movement_report report;
    } data;
    
};

APP_EVENT_TYPE_DECLARE(motor_module_event);