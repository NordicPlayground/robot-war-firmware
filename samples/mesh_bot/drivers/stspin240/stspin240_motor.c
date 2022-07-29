
#include <devicetree.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/pwm.h>
#include "../motors/motor.h"

#define DT_DRV_COMPAT st_stspin240_motor
#define STSPIN240_MOTOR_INIT_PRIORITY 60

#include <logging/log.h>
LOG_MODULE_REGISTER(stspin240_motor_driver, CONFIG_STSPIN240_MOTOR_DRIVER_LOG_LEVEL);

struct motor_data
{
};

struct motor_conf
{
    struct gpio_dt_spec phase_gpio;
    struct pwm_dt_spec pwm;
};

static int _drive_continous(const struct device *dev, int32_t power)
{
    struct motor_conf *conf = (struct motor_conf *)dev->config;

    if (conf->phase_gpio.port != NULL)
    {
        if (power >= 0)
        {
            gpio_pin_set_dt(&conf->phase_gpio, 1);
        }
        else if (power < 0)
        {
            gpio_pin_set_dt(&conf->phase_gpio, 0);
        }
    }
    else if (power < 0)
    {
        LOG_WRN("Negative power %d given but driver has no direction control GPIOs", power);
    }

    int32_t abs_power = power >= 0 ? power : -power;

    int err = pwm_set_pulse_dt(&conf->pwm, abs_power);
    if (err)
    {
        LOG_ERR("Failed to set PWM pulse: Error %d", err);
        return err;
    }

    LOG_DBG("Setting power on motor %s to %d", dev->name, abs_power);
    return 0;
}

struct motor_api api = {
    .drive_continous = _drive_continous,
    .set_position = NULL,
};

static int init_gpio(const struct device *dev)
{
    struct motor_conf *conf = (struct motor_conf *)dev->config;
    int err = 0;

    if (conf->phase_gpio.port != NULL)
    {
        if (!device_is_ready(conf->phase_gpio.port))
        {
            LOG_ERR("Phase gpio, %s, is not ready", conf->phase_gpio.port->name);
            return -ENODEV;
        }

        err = gpio_pin_configure_dt(&conf->phase_gpio, GPIO_OUTPUT);
        if (err)
        {
            LOG_ERR("Failed to configure phase gpio: Error %d", err);
            return err;
        }
    }

    return 0;
}

static int init_pwm(const struct device *dev)
{
    struct motor_conf *conf = (struct motor_conf *)dev->config;
    if (!device_is_ready(conf->pwm.dev))
    {
        LOG_ERR("PWM device not found or not ready");
        return -ENODEV;
    }
    int err = pwm_set_dt(&conf->pwm, conf->pwm.period, 0);
    if (err)
    {
        LOG_ERR("Failed to set PWM period %d ns: Error %d",conf->pwm.period, err);
        return err;
    }
    LOG_DBG("Channel %d on PWM %s initialized for motor %s", conf->pwm.channel, conf->pwm.dev->name, dev->name);
    return 0;
}

static int init_motor(const struct device *dev)
{
    int err = init_gpio(dev);
    if (err)
    {
        LOG_ERR("Error while initializig gpio: %d", err);
        return err;
    }
    err = init_pwm(dev);
    if (err)
    {
        LOG_ERR("Error while initializig pwm: %d", err);
        return err;
    }
    LOG_DBG("Motor %s initialized", dev->name);
    return 0;
}

#define INIT_STSPIN240_MOTOR(inst)                                      \
    static struct motor_conf conf_##inst = {                            \
        .phase_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, phase_gpios, {0}), \
        .pwm = PWM_DT_SPEC_GET(DT_INST(inst, DT_DRV_COMPAT)),           \
    };                                                                  \
    static struct motor_data data_##inst = {};                          \
    DEVICE_DT_INST_DEFINE(                                              \
        inst,                                                           \
        init_motor,                                                     \
        NULL,                                                           \
        &data_##inst,                                                   \
        &conf_##inst,                                                   \
        POST_KERNEL,                                                    \
        STSPIN240_MOTOR_INIT_PRIORITY,                                  \
        &api);

DT_INST_FOREACH_STATUS_OKAY(INIT_STSPIN240_MOTOR)