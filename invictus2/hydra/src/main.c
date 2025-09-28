#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

/* #include "valves.h" */
/* #include "pwm.h" */

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* const struct device *thermo1 = DEVICE_DT_GET(DT_NODELABEL(thermo1)); */
const struct device *thermo2 = DEVICE_DT_GET(DT_NODELABEL(thermo2));
const struct device *thermo3 = DEVICE_DT_GET(DT_NODELABEL(thermo3));

/* void setup() */
/* { */
/*     pwm_init(); */
/*     LOG_INF("PWM initialized"); */
/*     int rc = valves_init(); */
/*     if (rc) */
/*     { */
/*         LOG_WRN("Valves init failed: %d", rc); */
/*     } */
/*     else */
/*     { */
/*         LOG_INF("Valves initialized"); */
/*     } */
/* } */

/* void loop() */
/* { */
/*     k_sleep(K_MSEC(MAIN_DELAY)); */
/*     valve_pwm_set_angle(0.0f); */
/*     k_sleep(K_MSEC(MAIN_DELAY)); */
/*     valve_pwm_set_angle(180.0f); */
/* } */

int main(void)
{

    /* if (!device_is_ready(thermo1)) */
    /* { */
    /*     LOG_ERR("Thermocouple 1 device not ready"); */
    /*     return -1; */
    /* } */

    if (!device_is_ready(thermo2))
    {
        LOG_ERR("Thermocouple 2 device not ready");
        return -1;
    }

    if (!device_is_ready(thermo3))
    {
        LOG_ERR("Thermocouple 3 device not ready");
        return -1;
    }

    struct sensor_value env_temp1, env_temp2, env_temp3;
    struct sensor_value die_temp1, die_temp2, die_temp3;
    int ret1, ret2, ret3;

    (void)env_temp1;
    (void)die_temp1;
    (void)ret1;

    while (1)
    {
        ret2 = sensor_sample_fetch(thermo2);
        ret3 = sensor_sample_fetch(thermo3);

        k_sleep(K_MSEC(500));

        if (ret2 == 0)
        {
            sensor_channel_get(thermo2, SENSOR_CHAN_AMBIENT_TEMP, &env_temp2);
            sensor_channel_get(thermo2, SENSOR_CHAN_DIE_TEMP, &die_temp2);
            LOG_INF("T2 | Ambient: %d.%06d °C | Die: %d.%06d °C", env_temp2.val1,
                    env_temp2.val2, die_temp2.val1, die_temp2.val2);
        }
        else
        {
            LOG_ERR("Failed to fetch sample from Thermocouple 2: %d", ret2);
        }

        if (ret3 == 0)
        {
            sensor_channel_get(thermo3, SENSOR_CHAN_AMBIENT_TEMP, &env_temp3);
            sensor_channel_get(thermo3, SENSOR_CHAN_DIE_TEMP, &die_temp3);
            LOG_INF("T3 | Ambient: %d.%06d °C | Die: %d.%06d °C", env_temp2.val1,
                    env_temp2.val2, die_temp2.val1, die_temp2.val2);
        }
        else
        {
            LOG_ERR("Failed to fetch sample from Thermocouple 3: %d", ret3);
        }
    }
}
