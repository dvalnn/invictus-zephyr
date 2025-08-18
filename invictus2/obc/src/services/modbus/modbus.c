#include "services/modbus/modbus.h"

#include "services/modbus/internal/hydra.h"
#include "services/modbus/internal/lift.h"
#include "radio_commands.h"

#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include "zephyr/modbus/modbus.h"
#include "zephyr/usb/usb_device.h"
#include "zephyr/zbus/zbus.h"

LOG_MODULE_REGISTER(obc_modbus, LOG_LEVEL_DBG);

static void write_work_handler(struct k_work *work);
static void hydra_read_ir_work_handler(struct k_work *work);
static void lift_read_ir_work_handler(struct k_work *work);

static K_WORK_DEFINE(write_work, write_work_handler);
static K_WORK_DELAYABLE_DEFINE(hydra_sample_work, hydra_read_ir_work_handler);
static K_WORK_DELAYABLE_DEFINE(lift_sample_work, lift_read_ir_work_handler);

K_THREAD_STACK_DEFINE(modbus_work_q_stack, CONFIG_MODBUS_WORK_Q_STACK);
static struct k_work_q modbus_work_q;

// Published channels
ZBUS_CHAN_DECLARE(chan_thermo_sensors, chan_pressure_sensors, chan_weight_sensors);

// Subscribed channels
ZBUS_CHAN_DECLARE(chan_actuators, chan_radio_cmds);

static void modbus_listener_cb(const struct zbus_channel *chan)
{
    if (chan == &chan_actuators) {
        k_work_submit_to_queue(&modbus_work_q, &write_work);
        return;
    }

    if (chan == &chan_radio_cmds) {
        // TODO: implement worker
        /* k_work_submit_to_queue(&modbus_work_q, &write_work); */
        LOG_DBG("Received radio command"); // FIXME: remove
        return;
    }
}

ZBUS_LISTENER_DEFINE(modbus_listener, modbus_listener_cb);
ZBUS_CHAN_ADD_OBS(chan_actuators, modbus_listener, CONFIG_MODBUS_ZBUS_LISTENER_PRIO);

#define MODBUS_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_modbus_serial)

static struct hydra_boards hydras = {0};
static struct lift_boards lifts = {0};
static int client_iface;

static int rocket_state; // TODO

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
    const struct device *const dev = DEVICE_DT_GET(DT_PARENT(MODBUS_NODE));
    uint32_t dtr = 0;

    if (!device_is_ready(dev) || usb_enable(NULL)) {
        return false;
    }

    while (!dtr) {
        uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
        k_sleep(K_MSEC(100));
    }

    LOG_INF("Master connected on %s", dev->name);
#endif

    const char iface_name[] = {DEVICE_DT_NAME(MODBUS_NODE)};
    client_iface = modbus_iface_get_by_name(iface_name);

    if (client_iface < 0) {
        LOG_ERR("Failed to get client_iface index for %s", iface_name);
        return false;
    }

    return modbus_init_client(client_iface, client_param) == 0;
}

static void write_work_handler(struct k_work *work)
{
    k_oops(); // FIXME: implement function
}

static void hydra_read_ir_work_handler(struct k_work *work)
{
    k_work_schedule_for_queue(&modbus_work_q, &hydra_sample_work,
                              K_MSEC(CONFIG_MODBUS_HYDRA_SAMPLE_INTERVAL_MSEC));

    hydra_boards_read_irs(client_iface, &hydras);

    union thermocouples_u temperatures = {0};
    union pressures_u pressures = {0};
    hydra_boards_irs_to_zbus_rep(&hydras, &temperatures, &pressures);

    zbus_chan_pub(&chan_thermo_sensors, (const void *)&temperatures, K_NO_WAIT);
    zbus_chan_pub(&chan_pressure_sensors, (const void *)&pressures, K_NO_WAIT);
}

static void lift_read_ir_work_handler(struct k_work *work)
{
    k_work_schedule_for_queue(&modbus_work_q, &lift_sample_work,
                              K_MSEC(CONFIG_MODBUS_LIFT_SAMPLE_INTERVAL_MSEC));

    lift_boards_read_irs(client_iface, &lifts);

    union loadcell_weights_u weights = {0};
    lift_boards_irs_to_zbus_rep(&lifts, &weights);

    zbus_chan_pub(&chan_weight_sensors, (const void *)&weights, K_NO_WAIT);
}

void modbus_service_start(void)
{
    hydra_boards_init(&hydras);
    lift_boards_init(&lifts);

    k_work_queue_start(&modbus_work_q, modbus_work_q_stack,
                       K_THREAD_STACK_SIZEOF(modbus_work_q_stack), CONFIG_MODBUS_WORK_Q_PRIO,
                       NULL);

    k_work_schedule_for_queue(&modbus_work_q, &hydra_sample_work, K_NO_WAIT);
    k_work_schedule_for_queue(&modbus_work_q, &lift_sample_work, K_NO_WAIT);
}
