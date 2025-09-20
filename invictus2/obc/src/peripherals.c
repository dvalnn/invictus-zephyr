#include "peripherals.h"

#define PWM_NODE DT_NODELABEL(pwm)

LOG_MODULE_REGISTER(peripherals, LOG_LEVEL_INF);

const struct device *pwm_dev;

void pwm_set_duty_cycle(uint32_t channel, uint32_t period_us, uint32_t pulse_us) {

    if (!device_is_ready(pwm_dev)) {
        LOG_INF("PWM not ready!\n");
        return;
    }

    int ret = pwm_set(pwm_dev, channel, PWM_USEC(period_us), PWM_USEC(pulse_us), 0);
    if (ret) {
        if(ret == -ENOTSUP) {
            LOG_INF("Requested period or pulse cycles are not supported. !\n");
            return;
        }
        LOG_INF("Error %d: failed to set pulse width!\n", ret); 
        return;
    }
    LOG_INF("Successfully set pwm period %d us and pulse %d us on channel %d\n", period_us, pulse_us, channel);
}

void pwm_init(void) {
    pwm_dev = DEVICE_DT_GET(PWM_NODE);

    if (!device_is_ready(pwm_dev)) {
        LOG_INF("PWM not ready!\n");
        return;
    } 
    LOG_INF("PWM initialized\n");
}

