#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "valves.h"
#include "peripherals/pwm.h"
#include "peripherals/adc.h"
#include "pressures.h"

#define MAIN_DELAY 1000

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

void setup()
{
    pwm_init();
    LOG_INF("PWM initialized");
    init_adcs();
    LOG_INF("ADC initialized");
    int rc = valves_init();
    if (rc)
    {
        LOG_WRN("Valves init failed: %d", rc);
    }
    else
    {
        LOG_INF("Valves initialized");
    }
}

void loop()
{
    k_sleep(K_MSEC(MAIN_DELAY));
    read_adcs();
}

int main(void)
{
    setup();
    while (1)
    {
        loop();
    }
    k_oops(); // should never reach here
}