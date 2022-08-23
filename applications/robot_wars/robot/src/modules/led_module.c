
#include <zephyr.h>
#include <app_event_manager.h>
#include <devicetree.h>
#include <device.h>
#include <drivers/pwm.h>

#define MODULE led
#include "../events/mesh_module_event.h"
#include "../events/ui_module_event.h"


#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_LED_MODULE_LOG_LEVEL);

struct led_msg_data
{
    union
    {
        struct ui_module_event ui;
        struct mesh_module_event mesh;
    } event;
};

/* Led module message queue. */
#define LED_QUEUE_ENTRY_COUNT		10
#define LED_QUEUE_BYTE_ALIGNMENT	4

K_MSGQ_DEFINE(msgq_led, sizeof(struct led_msg_data),
	      LED_QUEUE_ENTRY_COUNT, LED_QUEUE_BYTE_ALIGNMENT);

static void blink_on_work_fn(struct k_work *work);
static void blink_off_work_fn(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(blink_on_work, blink_on_work_fn);
K_WORK_DELAYABLE_DEFINE(blink_off_work, blink_off_work_fn);

/* Global module data */

static const struct pwm_dt_spec red_pwm_led =
	PWM_DT_SPEC_GET(DT_ALIAS(red_pwm_led));
static const struct pwm_dt_spec green_pwm_led =
	PWM_DT_SPEC_GET(DT_ALIAS(green_pwm_led));
static const struct pwm_dt_spec blue_pwm_led =
	PWM_DT_SPEC_GET(DT_ALIAS(blue_pwm_led));

int blink_time = 500;
int r_val = 0;
int g_val = 0;
int b_val = 0;



/* Event handling */
static bool app_event_handler(const struct app_event_header *header)
{
    struct led_msg_data msg = {0};
    bool enqueue = false;

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
        int err = k_msgq_put(&msgq_led, &msg, K_FOREVER);
        if (err)
        {
            LOG_ERR("Message could not be enqueued");
        }
    }

    return false;
}

/* Led control */

static int set_led_value(const struct pwm_dt_spec *spec, uint8_t numerator, uint8_t denominator)
{
    uint32_t pulse;
    if (denominator == 0) {
        LOG_ERR("led denominator is zero!");
        return -EINVAL;
    }
    
    pulse = (spec->period/denominator)*numerator;
    LOG_DBG("Setting power on led to %d/%d with pulse width %d, period: %d", numerator, denominator, pulse, spec->period);
   
    return pwm_set_pulse_dt(spec, pulse);

}

static void blink_on_work_fn(struct k_work *work)
{
    set_led_value(&red_pwm_led, r_val, 255);
    set_led_value(&green_pwm_led, g_val, 255);
    set_led_value(&blue_pwm_led, b_val, 255);
    if (blink_time != 0) {
        k_work_schedule(&blink_off_work, K_MSEC(blink_time));
    }
}

static void blink_off_work_fn(struct k_work *work)
{
    set_led_value(&red_pwm_led, 0, 255);
    set_led_value(&green_pwm_led, 0, 255);
    set_led_value(&blue_pwm_led, 0, 255);
    k_work_schedule(&blink_on_work, K_MSEC(blink_time));
}

/* State handling*/
static int on_all_states(struct led_msg_data *msg)
{
    if (is_mesh_module_event((struct app_event_header *)(&msg->event.mesh)))
    {
        if (msg->event.mesh.type == MESH_EVT_RGB)
        {
            LOG_INF("rgb set event!");
            blink_time = msg->event.mesh.data.rgb.blink_time;
            r_val = msg->event.mesh.data.rgb.red;
            g_val = msg->event.mesh.data.rgb.green;
            b_val = msg->event.mesh.data.rgb.blue;
            k_work_reschedule(&blink_on_work, K_NO_WAIT);
        }
    }

    if (is_ui_module_event((struct app_event_header *)(&msg->event.ui)))
    {
        if (msg->event.ui.type == UI_EVT_BUTTON)
        {
            if (msg->event.ui.data.button.action == BUTTON_PRESS) {
                if (msg->event.ui.data.button.num == BTN3) {
                    blink_time = 1000;
                    r_val = 200;
                    g_val = 50;
                    b_val = 20;
                    k_work_schedule(&blink_on_work, K_MSEC(blink_time));
                } else if (msg->event.ui.data.button.num == BTN4) {
                    blink_time = 500;
                    r_val = 20;
                    g_val = 100;
                    b_val = 100;
                    k_work_schedule(&blink_on_work, K_MSEC(blink_time));
                }
                
            }
        }
    }
    return 0;
}

/* Setup */
static int init_leds()
{
    // LOG_DBG("Initializing led drivers");
    int err;
    err = !device_is_ready(red_pwm_led.dev);
    if (err)
    {
        LOG_ERR("red LED not ready: Error %d", err);
        return err;
    }

    err = !device_is_ready(green_pwm_led.dev);
    if (err)
    {
        LOG_ERR("red LED not ready: Error %d", err);
        return err;
    }

    err = !device_is_ready(blue_pwm_led.dev);
    if (err)
    {
        LOG_ERR("red LED not ready: Error %d", err);
        return err;
    }

    blink_time = 1000;
    r_val = 10;
    g_val = 10;
    b_val = 250;
    k_work_schedule(&blink_on_work, K_MSEC(blink_time));

    return 0;
}

/* Module thread */
static void module_thread_fn(void)
{
    LOG_INF("led module thread started");
    struct led_msg_data msg;

    int err;

    err = init_leds();
    if (err)
    {
        
        return;
    }

    while (true)
    {

        k_msgq_get(&msgq_led, &msg, K_FOREVER);
        on_all_states(&msg);

    }
}

K_THREAD_DEFINE(led_module_thread, CONFIG_LED_THREAD_STACK_SIZE,
    module_thread_fn, NULL, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, mesh_module_event);
APP_EVENT_SUBSCRIBE(MODULE, ui_module_event);