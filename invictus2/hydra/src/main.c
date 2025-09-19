#include <zephyr/kernel.h>
#include "valves.h"
#include "pwm.h"

#define MAIN_DELAY 1000

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

void setup() {
    pwm_init();
    LOG_INF("PWM initialized");
}

void loop() {
    k_sleep(K_MSEC(MAIN_DELAY));
    valve_pwm_set_angle(0.0f);
    k_sleep(K_MSEC(MAIN_DELAY));
    valve_pwm_set_angle(180.0f);
}

int main(void)
{
    setup();
    while(1) {
        loop();
    }
    k_oops(); // should never reach here
}