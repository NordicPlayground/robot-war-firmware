#include <zephyr/device.h>

/**
 * @brief Set duty cycle for the current limiter PWM.
 * 
 * @param duty_cycle 
 * @return 0 on success, negative error code otherwise
 */
int set_current_lim_duty_cycle(const struct device *dev, int duty_cycle);