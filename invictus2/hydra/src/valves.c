// HYDRA valves control implementation
#include "valves.h"

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include "peripherals/gpio.h"
#include "data_models.h"

LOG_MODULE_REGISTER(valves, LOG_LEVEL_INF);

static bool valve_states[VALVE_COUNT];

int valves_init(void)
{
    memset(valve_states, 0, sizeof(valve_states));
    int rc = gpio_init_valves();
    if (rc == 0) {
        LOG_INF("Valves initialized (%d)", VALVE_COUNT);
    }
    return rc;
}

int valve_set(valve_t id, bool open)
{
    if (id < 0 || id >= VALVE_COUNT) {
        return -EINVAL;
    }

    int r = gpio_set_valve((int)id, open);
    if (r == 0) {
        valve_states[id] = open;
        LOG_DBG("Valve %u set to %s", id, open ? "OPEN" : "CLOSED");
    }
    return r;
}

int valve_toggle(valve_t id)
{
    if (id < 0 || id >= VALVE_COUNT) {
        return -EINVAL;
    }
    return valve_set(id, !valve_states[id]);
}

bool valve_is_open(valve_t id)
{
    if (id < 0 || id >= VALVE_COUNT) {
        return -EINVAL;
    }
    return valve_states[id];
}

int valve_open_ms(valve_t id, uint32_t ms)
{
    int r = valve_set(id, true);
    if (r) return r;
    k_msleep(ms);
    return valve_set(id, false);
}

/* SERVO VALVE WONT BE USED FOR NOW
// Sets the servo angle in degrees [0, 270]
void valve_pwm_set_angle(float angle)
{
    if (angle < 0) angle = 0;
    if (angle > 270) angle = 270;

    const int minPulse = 500;   // microseconds
    const int maxPulse = 2500;  // microseconds
    const int angleRange = 270;

    int pulseWidth = minPulse + (int)((angle / angleRange) * (maxPulse - minPulse));

    pwm_set_duty_cycle(5, 20000, pulseWidth); // Channel 5, 20ms period, pulse width calculated above
}
*/
