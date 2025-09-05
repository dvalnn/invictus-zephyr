#include "services/rocket_state.h"

#include "zephyr/kernel.h"
#include "zephyr/zbus/zbus.h"
#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(rocket_state_service, LOG_LEVEL_DBG);

//* K_THREAD_STACK_DEFINE(rocket_state_work_q_stack, CONFIG_ROCKET_STATE_WORK_Q_STACK); */
K_THREAD_STACK_DEFINE(rocket_state_work_q_stack, 1024); // FIXME: Make KConfig
static struct k_work_q rocket_state_work_q;

static void radio_cmd_work_handler(struct k_work *work);
static void weight_work_handler(struct k_work *work);
static void thermo_work_handler(struct k_work *work);
static void pressure_work_handler(struct k_work *work);

static K_WORK_DEFINE(radio_cmd_work, radio_cmd_work_handler);
static K_WORK_DEFINE(weight_work, weight_work_handler);
static K_WORK_DEFINE(thermo_work, thermo_work_handler);
static K_WORK_DEFINE(pressure_work, pressure_work_handler);

// Subscribed channels
ZBUS_CHAN_DECLARE(chan_radio_cmds);
ZBUS_CHAN_DECLARE(chan_weight_sensors, chan_pressure_sensors, chan_thermo_sensors);

// Published channels
ZBUS_CHAN_DECLARE(chan_rocket_state);

static void rocket_state_listener_cb(const struct zbus_channel *chan);

ZBUS_LISTENER_DEFINE(rocket_state_listener, rocket_state_listener_cb);
ZBUS_CHAN_ADD_OBS(chan_radio_cmds, rocket_state_listener, 5);       // FIXME: Make KConfig
ZBUS_CHAN_ADD_OBS(chan_weight_sensors, rocket_state_listener, 5);   // FIXME: Make KConfig
ZBUS_CHAN_ADD_OBS(chan_thermo_sensors, rocket_state_listener, 5);   // FIXME: Make KConfig
ZBUS_CHAN_ADD_OBS(chan_pressure_sensors, rocket_state_listener, 5); // FIXME: Make KConfig

static void rocket_state_listener_cb(const struct zbus_channel *chan)
{
    if (chan == &chan_radio_cmds) {
        k_work_submit_to_queue(&rocket_state_work_q, &radio_cmd_work);
        return;
    }

    if (chan == &chan_weight_sensors) {
        k_work_submit_to_queue(&rocket_state_work_q, &weight_work);
        return;
    }

    if (chan == &chan_thermo_sensors) {
        k_work_submit_to_queue(&rocket_state_work_q, &thermo_work);
        return;
    }

    if (chan == &chan_pressure_sensors) {
        k_work_submit_to_queue(&rocket_state_work_q, &pressure_work);
        return;
    }
}

static void radio_cmd_work_handler(struct k_work *work)
{
    k_oops(); // FIXME:
}

static void weight_work_handler(struct k_work *work)
{
    k_oops(); // FIXME:
}

static void thermo_work_handler(struct k_work *work)
{
    k_oops(); // FIXME:
}

static void pressure_work_handler(struct k_work *work)
{
    k_oops(); // FIXME:
}

bool rocket_state_service_setup(void)
{
    LOG_WRN_ONCE("Service setup is not implemented");
    return true;
}

void rocket_state_service_start(void)
{
    LOG_WRN_ONCE("Service is not implemented.");
}
