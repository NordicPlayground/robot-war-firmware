#pragma once

#include <app_event_manager.h>

typedef enum {
    MESH_EVT_PROVISIONED,
    MESH_EVT_DISCONNECTED,
    MESH_EVT_MOVE,
    MESH_EVT_RGB,
} mesh_module_event_type;

struct mesh_module_event {
    struct app_event_header header;
    mesh_module_event_type type;
    union {
        struct bt_mesh_movement_set move; // Should only be read when type == MESH_EVT_MOVEMENT_RECEIVED
        struct bt_mesh_light_rgb_set rgb;
    } data;
};

APP_EVENT_TYPE_DECLARE(mesh_module_event);