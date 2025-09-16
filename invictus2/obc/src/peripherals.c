#include "peripherals.h"

#define PWM_NODE DT_NODELABEL(pwm)

LOG_MODULE_REGISTER(peripherals, LOG_LEVEL_INF);

void pwm_init(void) {
    const struct device *pwm_dev = DEVICE_DT_GET(PWM_NODE);

    if (!device_is_ready(pwm_dev)) {
        LOG_INF("PWM not ready!\n");
        return;
    }

    // Example: channel 5, period 20 ms, duty 50%
    pwm_set(pwm_dev, 5, PWM_MSEC(20), PWM_MSEC(10), 0);
}

