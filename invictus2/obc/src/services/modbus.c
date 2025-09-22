#include "services/modbus.h"
#include "services/modbus/hydra.h"
#include "services/modbus/lift.h"

#include "data_models.h"

#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include "zephyr/modbus/modbus.h"
#include "zephyr/zbus/zbus.h"

#define MODBUS_NODE DT_ALIAS(modbus_rtu)

#if DT_NODE_HAS_COMPAT(DT_PARENT(MODBUS_NODE), zephyr_cdc_acm_uart)
#include "zephyr/usb/usb_device.h"
#endif

LOG_MODULE_REGISTER(modbus_service, LOG_LEVEL_DBG);

static void lift_read_ir_work_handler(struct k_work *work);
static void hydra_read_ir_work_handler(struct k_work *work);

static void actuator_work_handler(struct k_work *work);
static void command_work_handler(struct k_work *work);
static void rocket_state_work_handler(struct k_work *work);

static K_WORK_DEFINE(actuator_work, actuator_work_handler);
static K_WORK_DEFINE(command_work, command_work_handler);
static K_WORK_DEFINE(rocket_state_work, rocket_state_work_handler);

static K_WORK_DELAYABLE_DEFINE(lift_sample_work, lift_read_ir_work_handler);
static K_WORK_DELAYABLE_DEFINE(hydra_sample_work, hydra_read_ir_work_handler);

K_THREAD_STACK_DEFINE(modbus_work_q_stack, CONFIG_MODBUS_WORK_Q_STACK);
static struct k_work_q modbus_work_q;

// Published channels
ZBUS_CHAN_DECLARE(chan_thermo_sensors, chan_pressure_sensors, chan_weight_sensors);

// Subscribed channels
ZBUS_CHAN_DECLARE(chan_actuators, chan_packets, chan_rocket_state);

static void modbus_listener_cb(const struct zbus_channel *chan);

ZBUS_LISTENER_DEFINE(modbus_listener, modbus_listener_cb);
ZBUS_CHAN_ADD_OBS(chan_actuators, modbus_listener, CONFIG_MODBUS_ZBUS_LISTENER_PRIO);
ZBUS_CHAN_ADD_OBS(chan_packets, modbus_listener, CONFIG_MODBUS_ZBUS_LISTENER_PRIO);
ZBUS_CHAN_ADD_OBS(chan_rocket_state, modbus_listener, CONFIG_MODBUS_ZBUS_LISTENER_PRIO);

// Static Variables
//
static struct hydra_boards hydras = {0};
static struct lift_boards lifts = {0};
static int client_iface;
static atomic_t fs_disabled = ATOMIC_INIT(false);

// Function Implementations

static void modbus_listener_cb(const struct zbus_channel *chan)
{
    if (chan == &chan_actuators)
    {
        k_work_submit_to_queue(&modbus_work_q, &actuator_work);
        return;
    }

    if (chan == &chan_packets)
    {
        k_work_submit_to_queue(&modbus_work_q, &command_work);
        return;
    }

    // NOTE: There is no need to listen to the rocket state after the filling station is
    // disabled.
    if (chan == &chan_rocket_state && !(bool)atomic_get(&fs_disabled))
    {
        k_work_submit_to_queue(&modbus_work_q, &rocket_state_work);
        return;
    }
}

const static struct modbus_iface_param client_param = {
    .mode = MODBUS_MODE_RTU,
    .rx_timeout = 50000, // TODO Make KCONFIG
    .serial =
        {
            .baud = 115200,
            // NOTE: In RTU mode, modbus uses CRC checks, so parity can be NONE
            .parity = UART_CFG_PARITY_NONE,
            .stop_bits_client = UART_CFG_STOP_BITS_1,
        },
};

bool modbus_service_setup(void)
{
#if DT_NODE_HAS_COMPAT(DT_PARENT(MODBUS_NODE), zephyr_cdc_acm_uart)
    LOG_INF("Waiting for Modbus RTU Master connection on %s...",
            DEVICE_DT_NAME(DT_PARENT(MODBUS_NODE)));

    const struct device *const dev = DEVICE_DT_GET(DT_PARENT(MODBUS_NODE));
    uint32_t dtr = 0;

    if (!device_is_ready(dev) || usb_enable(NULL))
    {
        return false;
    }

    while (!dtr)
    {
        uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
        k_sleep(K_MSEC(100));
    }

    LOG_INF("Master connected on %s", dev->name);
#endif

    const char iface_name[] = {DEVICE_DT_NAME(MODBUS_NODE)};
    client_iface = modbus_iface_get_by_name(iface_name);

    if (client_iface < 0)
    {
        LOG_ERR("Failed to get client_iface index for %s", iface_name);
        return false;
    }

    return modbus_init_client(client_iface, client_param) == 0;
}

static void hydra_read_ir_work_handler(struct k_work *work)
{
    k_work_schedule_for_queue(&modbus_work_q, &hydra_sample_work,
                              K_MSEC(CONFIG_MODBUS_HYDRA_SAMPLE_INTERVAL_MSEC));

    thermocouples_t temperatures = {0};
    pressures_t pressures = {0};
    hydra_boards_read_irs(client_iface, &hydras, (bool)atomic_get(&fs_disabled));
    hydra_boards_irs_to_zbus_rep(&hydras, &temperatures, &pressures,
                                 (bool)atomic_get(&fs_disabled));

    zbus_chan_pub(&chan_thermo_sensors, (const void *)&temperatures, K_MSEC(100));
    zbus_chan_pub(&chan_pressure_sensors, (const void *)&pressures, K_MSEC(100));
}

static void lift_read_ir_work_handler(struct k_work *work)
{
    k_work_schedule_for_queue(&modbus_work_q, &lift_sample_work,
                              K_MSEC(CONFIG_MODBUS_LIFT_SAMPLE_INTERVAL_MSEC));

    loadcell_weights_t weights = {0};
    lift_boards_read_irs(client_iface, &lifts, (bool)atomic_get(&fs_disabled));
    lift_boards_irs_to_zbus_rep(&lifts, &weights, (bool)atomic_get(&fs_disabled));

    zbus_chan_pub(&chan_weight_sensors, (const void *)&weights, K_MSEC(100));
}

static void actuator_work_handler(struct k_work *work)
{
    k_oops(); // FIXME: implement function
}

static void command_work_handler(struct k_work *work)
{
    k_oops(); // FIXME: implement function
}

static void rocket_state_work_handler(struct k_work *work)
{
    k_oops(); // FIXME: implement function
}

void modbus_service_start(void)
{
    hydra_boards_init(&hydras);
    lift_boards_init(&lifts);

    k_work_queue_start(&modbus_work_q, modbus_work_q_stack,
                       K_THREAD_STACK_SIZEOF(modbus_work_q_stack), CONFIG_MODBUS_WORK_Q_PRIO,
                       NULL);

    k_work_schedule_for_queue(&modbus_work_q, &hydra_sample_work, K_MSEC(100));
    k_work_schedule_for_queue(&modbus_work_q, &lift_sample_work, K_MSEC(100));
}
