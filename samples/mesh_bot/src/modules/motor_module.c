
#include <zephyr.h>
#include <app_event_manager.h>
#include <devicetree.h>
#include <device.h>
#include <stdlib.h>

#define MODULE motor
#include "../events/mesh_module_event.h"
#include "../events/motor_module_event.h"
#include "../events/ui_module_event.h"

#include "../../drivers/motors/motor.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_MOTOR_MODULE_LOG_LEVEL);

struct motor_msg_data
{
    union
    {
        struct ui_module_event ui;
        struct mesh_module_event mesh;
        struct motor_module_event motor;
    } event;
};

/* Motor module message queue. */
#define MOTOR_QUEUE_ENTRY_COUNT		10
#define MOTOR_QUEUE_BYTE_ALIGNMENT	4

K_MSGQ_DEFINE(msgq_motor, sizeof(struct motor_msg_data),
	      MOTOR_QUEUE_ENTRY_COUNT, MOTOR_QUEUE_BYTE_ALIGNMENT);

struct bt_mesh_movement_set movement;

/* motor module super states. */
enum state_type
{
    STATE_MOTOR_STANDBY,
    STATE_MOTOR_TURNING,
    STATE_MOTOR_MOVING, 
} state;

/* Global module data */
const struct device *device_motor_a = DEVICE_DT_GET(DT_ALIAS(motora));
const struct device *device_motor_b = DEVICE_DT_GET(DT_ALIAS(motorb));

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type state)
{
	switch (state)
	{
	case STATE_MOTOR_STANDBY:
		return "STATE_MOTOR_STANDBY";
	case STATE_MOTOR_TURNING:
		return "STATE_MOTOR_TURNING";
	case STATE_MOTOR_MOVING:
		return "STATE_MOTOR_MOVING";
	default:
		return "Unknown state";
	}
}

static void state_set(enum state_type new_state)
{
	if (new_state == state)
	{
		LOG_INF("State: %s", state2str(state));
		return;
	}

	LOG_INF("State transition %s --> %s",
			state2str(state),
			state2str(new_state));

	state = new_state;
}

/* Event handling */
static bool app_event_handler(const struct app_event_header *header)
{
    struct motor_msg_data msg = {0};
    bool enqueue = false;

    if (is_motor_module_event(header))
    {
        msg.event.motor = *cast_motor_module_event(header);
        enqueue = true;
    }

    if (is_mesh_module_event(header))
    {
        msg.event.mesh = *cast_mesh_module_event(header);
        enqueue = true;
    }

    if (is_ui_module_event(header))
    {
        msg.event.ui = *cast_ui_module_event(header);
        enqueue = true;
    }

    if (enqueue)
    {
        int err = k_msgq_put(&msgq_motor, &msg, K_FOREVER);
        if (err)
        {
            LOG_ERR("Message could not be enqueued");
        }
    }

    return false;
}

/* Motor actuation */
static void stop_motor_work_fn(struct k_work *work)
{
    motor_drive_continous(device_motor_a, 0, 100, 1);
    motor_drive_continous(device_motor_b, 0, 100, 1);
    struct motor_module_event *event = new_motor_module_event();
    event->type = MOTOR_EVT_MOVEMENT_DONE;
    APP_EVENT_SUBMIT(event);
}

K_WORK_DELAYABLE_DEFINE(stop_motor_work, stop_motor_work_fn);

static int turn_degrees(int32_t angle)
{
    if (angle == 0) {
        k_work_schedule(&stop_motor_work, K_USEC(1000));
        return 0;
    }

    if (abs(angle) > 179) {
        angle = 180;
    }

    if (angle < 0)
    {
        motor_drive_continous(device_motor_a, 100, 100, 1);
        motor_drive_continous(device_motor_b, 100, 100, 0);
    } else {
        motor_drive_continous(device_motor_a, 100, 100, 0);
        motor_drive_continous(device_motor_b, 100, 100, 1);
    }
    
    k_work_schedule(&stop_motor_work, K_USEC(abs(angle)*3000));
    return 0;
}

static int drive_forward(uint32_t time, uint8_t speed)
{
    motor_drive_continous(device_motor_a, speed, 100, 1);
    motor_drive_continous(device_motor_b, speed, 100, 1);
    k_work_schedule(&stop_motor_work, K_MSEC(time));
    return 0;
}

/* State handling*/
static int on_state_standby(struct motor_msg_data *msg)
{
    if (is_mesh_module_event((struct app_event_header *)(&msg->event.mesh)))
    {
        if (msg->event.mesh.type == MESH_EVT_MOVE)
        {
            movement = msg->event.mesh.data.move;
            state_set(STATE_MOTOR_TURNING);
            turn_degrees(msg->event.mesh.data.move.angle);
        }
    }
    return 0;
}

static int on_state_turning(struct motor_msg_data *msg)
{
    if (is_motor_module_event((struct app_event_header *)(&msg->event.motor)))
    {
        if (msg->event.motor.type == MOTOR_EVT_MOVEMENT_DONE)
        {
            state_set(STATE_MOTOR_MOVING);
            drive_forward(movement.time, movement.speed);
        }
    }
    return 0;
}

static int on_state_moving(struct motor_msg_data *msg)
{
        {
            return 0;
        }
        }
    }
    return 0;
}

static int on_all_states(struct motor_msg_data *msg)
{
    if (is_ui_module_event((struct app_event_header *)(&msg->event.ui)))
    {
        if (msg->event.ui.type == UI_EVT_BUTTON)
        {
            if (msg->event.ui.data.button.action == BUTTON_PRESS) {
                if (msg->event.ui.data.button.num == BTN1) {
                    motor_drive_continous(device_motor_a, 100, 100, 1);
                    motor_drive_continous(device_motor_b, 100, 100, 1);
                } else if (msg->event.ui.data.button.num == BTN2) {
                    motor_drive_continous(device_motor_a, 0, 100, 1);
                    motor_drive_continous(device_motor_b, 0, 100, 1);
                } else if (msg->event.ui.data.button.num == BTN3) {
                    motor_drive_continous(device_motor_a, 100, 100, 0);
                    motor_drive_continous(device_motor_b, 100, 100, 0);
                }
            }
        }
    }
    return 0;
}

/* Setup */
static int init_motors()
{
    // LOG_DBG("Initializing motor drivers");
    int err;
    err = !device_is_ready(device_motor_a);
    if (err)
    {
        LOG_ERR("Motor a not ready: Error %d", err);
        return err;
    }

    err = !device_is_ready(device_motor_b);
    if (err)
    {
        LOG_ERR("Motor b not ready: Error %d", err);
        return err;
    }
    return 0;
}

/* Module thread */
static void module_thread_fn(void)
{
    LOG_INF("motor module thread started");
    struct motor_msg_data msg;

    int err;

    err = init_motors();
    if (err)
    { 
        return;
    }
    state_set(STATE_MOTOR_STANDBY);

    while (true)
    {
        k_msgq_get(&msgq_motor, &msg, K_FOREVER);
        
        switch (state)
        {
            case STATE_MOTOR_STANDBY:
            {
                on_state_standby(&msg);
                break;
            }
            case STATE_MOTOR_TURNING:
            {
                on_state_turning(&msg);
                break;
            }
            case STATE_MOTOR_MOVING:
            {
                on_state_moving(&msg);
                break;
            }
            default:
            {
                LOG_ERR("Unknown motor module state %d", state);
            }
        }
        on_all_states(&msg);
    }
}

K_THREAD_DEFINE(motor_module_thread, CONFIG_MOTOR_THREAD_STACK_SIZE,
    module_thread_fn, NULL, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, mesh_module_event);
APP_EVENT_SUBSCRIBE(MODULE, motor_module_event);
APP_EVENT_SUBSCRIBE(MODULE, ui_module_event);