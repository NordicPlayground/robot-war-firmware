
#include <zephyr.h>
#include <app_event_manager.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/mesh/dk_prov.h>
#include <zephyr/bluetooth/mesh/msg.h>
#include <bluetooth/mesh/models.h>



#define MODULE mesh
#include "../events/module_state_event.h"
#include "../events/motor_module_event.h"
#include "../events/mesh_module_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_MESH_MODULE_LOG_LEVEL);


static struct k_sem settings_loaded; 
/* State handling*/

// Top level module states
static enum state_type
{
    STATE_UNPROVISIONED,
    STATE_PROVISIONED,
    STATE_READY_TO_MOVE,
    STATE_MOVING,
} state;

/** Message item that holds any received events */
struct mesh_msg_data
{
    union
    {
        struct motor_module_event motor;
    } event;
};

/* Modem module message queue. */
#define MESH_QUEUE_ENTRY_COUNT		10
#define MESH_QUEUE_BYTE_ALIGNMENT	4

/** Module message queue */
K_MSGQ_DEFINE(mesh_module_msg_q, sizeof(struct mesh_msg_data),
        MESH_QUEUE_ENTRY_COUNT, MESH_QUEUE_BYTE_ALIGNMENT);

/** Globals */
bt_addr_le_t addr;

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type state)
{
	switch (state) {
	case STATE_UNPROVISIONED:
		return "STATE_UNPROVISIONED";
	case STATE_PROVISIONED:
		return "STATE_PROVISIONED";
	case STATE_READY_TO_MOVE:
		return "STATE_READY_TO_MOVE";
	case STATE_MOVING:
		return "STATE_MOVING";
	default:
		return "Unknown state";
	}
}

static void state_set(enum state_type new_state)
{
    if (new_state == state) {
		LOG_DBG("State: %s", state2str(state));
		return;
	}

	LOG_DBG("State transition %s --> %s",
		state2str(state),
		state2str(new_state));

	state = new_state;
}

/* Mesh handlers */
static bool app_event_handler(const struct app_event_header *header)
{
    struct mesh_msg_data msg = {0};
    bool enqueue = false;

    if (is_motor_module_event(header))
    {
        struct motor_module_event *evt = cast_motor_module_event(header);
        msg.event.motor = *evt;
        enqueue = true;
    }

    if (enqueue)
    {
        int err = k_msgq_put(&mesh_module_msg_q, &msg, K_FOREVER);
        if (err)
        {
            LOG_ERR("Message could not be enqueued");
        }
    }

    return false;
}

/* SIG models */
static void attention_on(struct bt_mesh_model *model)
{
    printk("attention_on()\n");
}

static void attention_off(struct bt_mesh_model *model)
{
    printk("attention_off()\n");
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
    .attn_on = attention_on,
    .attn_off = attention_off,
};

static struct bt_mesh_health_srv health_srv = {
    .cb = &health_srv_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

static uint8_t * handle_robot_identify(struct bt_mesh_robot_srv *srv)
{
    size_t count;
    
    bt_id_get(&addr, &count);
    if (count == 0) {
        LOG_ERR("No ids found");
        return NULL;
    }
    
    return addr.a.val;
}

static void handle_robot_move (struct bt_mesh_robot_srv *srv,
					  struct bt_mesh_movement_set *movement) 
{
    struct mesh_module_event *event = new_mesh_module_event();
    event->type = MESH_EVT_MOVE;
    event->data.move.time = movement->time;
    event->data.move.angle = movement->angle;
    event->data.move.speed = movement->speed;
    APP_EVENT_SUBMIT(event);
    
}

static const struct bt_mesh_robot_srv_handlers robot_cb = {
	.identify = handle_robot_identify,
    .move = handle_robot_move,
};

static struct bt_mesh_robot_srv robot = {
    .handlers = &robot_cb,
};


static void handle_light_rgb_set (struct bt_mesh_light_rgb_srv *srv, 
			struct bt_mesh_light_rgb_set *rgb) 
{
    struct mesh_module_event *event = new_mesh_module_event();
    event->type = MESH_EVT_RGB;
    event->data.rgb.red = rgb->red;
    event->data.rgb.green = rgb->green;
    event->data.rgb.blue = rgb->blue;
    event->data.rgb.blink_time = rgb->blink_time;
    APP_EVENT_SUBMIT(event);
}

static const struct bt_mesh_light_rgb_srv_handlers light_rgb_cb = {
	.set = handle_light_rgb_set,
};

static struct bt_mesh_light_rgb_srv light_rgb = {
    .handlers = &light_rgb_cb,
};

/* Composition */
static struct bt_mesh_elem elements[] = {
    BT_MESH_ELEM(
		0,
        BT_MESH_MODEL_LIST(
            BT_MESH_MODEL_CFG_SRV,
            BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub)
        ),
        BT_MESH_MODEL_LIST( 
            BT_MESH_MODEL_ROBOT_SRV(&robot),
            BT_MESH_MODEL_LIGHT_RGB_SRV(&light_rgb)
        )
    )
};

static struct bt_mesh_comp comp = {
    .cid = CONFIG_BT_COMPANY_ID,
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements),
};

static void on_all_states(struct mesh_msg_data *msg)
{
    if (is_motor_module_event((struct app_event_header *)(&msg->event.motor)))
    {
        if (msg->event.motor.type == MOTOR_EVT_MOVEMENT_REPORT)
        {
            struct bt_mesh_telemetry_report telemetry = {
                .revolutions = msg->event.motor.data.report.revolutions,
            };
            
            bt_mesh_robot_report_telemetry(&robot, telemetry);
        }
    }
}

/* Setup */
static int setup_mesh(void)
{
    int err;
    k_sem_init(&settings_loaded, 0, 1);
    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Failed to initialize bluetooth: Error %d", err);
        return err;
    }
    LOG_DBG("Bluetooth initialized");

    err = bt_mesh_init(bt_mesh_dk_prov_init(), &comp);
    if (err) {
        LOG_ERR("Failed to initialize mesh: Error %d", err);
        return err;
    }
    
    if (IS_ENABLED(CONFIG_SETTINGS)) {
        err = settings_load();
        if (err) {
            LOG_ERR("Failed to load settings: Error %d", err);
        }
    }

    err = bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
    if (err == -EALREADY) {
        LOG_DBG("Device already provisioned");
        state_set(STATE_PROVISIONED);
        struct mesh_module_event *evt = new_mesh_module_event();
        evt->type = MESH_EVT_PROVISIONED;
        APP_EVENT_SUBMIT(evt);
    }
    LOG_DBG("Mesh initialized");

    return 0;
}



/* Module thread */
static void module_thread_fn(void)
{
    // LOG_DBG("Mesh module thread started");
    struct mesh_msg_data msg;
    // k_sleep(K_MSEC(200));
    // LOG_DBG("Skjer");
    int err;

    err = setup_mesh();

    if (err) {
        LOG_ERR("Failed to set up mesh. Error %d", err);
        return;
    }

    while (true) {
        k_msgq_get(&mesh_module_msg_q, &msg, K_FOREVER);

        switch(state) {
            case STATE_UNPROVISIONED: {
                break;
            }
            case STATE_PROVISIONED: {
                break;
            }
            case STATE_READY_TO_MOVE: {
                break;
            }
            case STATE_MOVING: {
                break;
            }
            default: {
                LOG_ERR("Unknown mesh module state %d", state);
            }
        }
        on_all_states(&msg);
    }
}

K_THREAD_DEFINE(mesh_module_thread, CONFIG_MESH_THREAD_STACK_SIZE,module_thread_fn,
    NULL, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

/* Event handling */
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, motor_module_event);