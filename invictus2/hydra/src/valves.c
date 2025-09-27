#include <string.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/zbus/zbus.h>

#include "peripherals/gpio.h"
#include "data_models.h"
#include "valves.h"

LOG_MODULE_REGISTER(valves, LOG_LEVEL_INF);

// Subscribed channels
ZBUS_CHAN_DECLARE(chan_valves);

static void valves_listener_cb(const struct zbus_channel *chan);

ZBUS_LISTENER_DEFINE(valves_listener, valves_listener_cb);
ZBUS_CHAN_ADD_OBS(chan_valves, valves_listener, CONFIG_VALVES_ZBUS_LISTENER_PRIO);

K_THREAD_STACK_DEFINE(valves_work_q_stack, CONFIG_VALVES_WORK_Q_STACK);
static struct k_work_q valves_work_q;

static void valves_work_handler(struct k_work *work);
static K_WORK_DEFINE(valves_work, valves_work_handler);

static bool valve_states[VALVE_COUNT];

static void valves_listener_cb(const struct zbus_channel *chan)
{
    if (chan == NULL)
    {
        return;
    }

    if (chan == &chan_valves)
    {
        const valves_msg_t *msg = zbus_chan_const_msg(chan);
        if (msg == NULL)
        {
            return;
        }
        k_work_submit_to_queue(&valves_work_q, &valves_work);
        return;
    }
}

static void valves_work_handler(struct k_work *work)
{
    valves_msg_t msg;
    zbus_chan_read(&chan_valves, &msg, K_NO_WAIT);
    for (size_t i = 0; i < VALVE_COUNT; i++)
    {
        valve_set((valve_t)i, msg.valve_states[i]);
    }
}

int valves_init(void)
{
    memset(valve_states, 0, sizeof(valve_states));
    int rc = gpio_init_valves();
    if (rc == 0)
    {
        LOG_INF("Valves initialized (%d)", VALVE_COUNT);
    }
    return rc;
}

int valve_set(valve_t id, bool open)
{
    if (id < 0 || id >= VALVE_COUNT)
    {
        return -EINVAL;
    }

    int r = gpio_set_valve((int)id, open);
    if (r == 0)
    {
        valve_states[id] = open;
        LOG_DBG("Valve %u set to %s", id, open ? "OPEN" : "CLOSED");
    }
    return r;
}

int valve_toggle(valve_t id)
{
    if (id < 0 || id >= VALVE_COUNT)
    {
        return -EINVAL;
    }
    return valve_set(id, !valve_states[id]);
}

bool valve_is_open(valve_t id)
{
    if (id < 0 || id >= VALVE_COUNT)
    {
        return -EINVAL;
    }
    return valve_states[id];
}

int valve_open_ms(valve_t id, uint32_t ms)
{
    int r = valve_set(id, true);
    if (r)
    {
        return r;
    }
    k_msleep(ms);
    return valve_set(id, false);
}

void valves_start(void)
{
    k_work_queue_start(&valves_work_q, valves_work_q_stack,
                       K_THREAD_STACK_SIZEOF(valves_work_q_stack), CONFIG_VALVES_WORK_Q_PRIO,
                       NULL);
}
