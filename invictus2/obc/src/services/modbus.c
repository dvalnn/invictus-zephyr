#include "services/modbus.h"

#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include "zephyr/modbus/modbus.h"
#include "zephyr/usb/usb_device.h"
#include "zephyr/zbus/zbus.h"

#include "hydra.h"
#include "lift.h"
#include "zbus_messages.h"

LOG_MODULE_REGISTER(obc_modbus, LOG_LEVEL_DBG);

static void write_work_handler(struct k_work *work);
static void rocket_hydra_sample_work_handler(struct k_work *work);

static K_WORK_DEFINE(write_work, write_work_handler);
static K_WORK_DELAYABLE_DEFINE(rocket_hydra_sample_work, rocket_hydra_sample_work_handler);

K_THREAD_STACK_DEFINE(modbus_work_q_stack, CONFIG_MODBUS_WORK_Q_STACK);
static struct k_work_q modbus_work_q;

ZBUS_CHAN_DECLARE(uf_hydra_chan, lf_hydra_chan);
ZBUS_CHAN_DECLARE(modbus_write_coils_chan);

static void listener_cb(const struct zbus_channel *chan)
{
    if (chan == &modbus_write_coils_chan) {
        k_work_submit_to_queue(&modbus_work_q, &write_work);
    }
}

ZBUS_LISTENER_DEFINE(modbus_listener, listener_cb);
ZBUS_CHAN_ADD_OBS(modbus_write_coils_chan, modbus_listener, CONFIG_MODBUS_ZBUS_LISTENER_PRIO);

#define MODBUS_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_modbus_serial)

static int client_iface;

const static struct modbus_iface_param client_param = {
    .mode = MODBUS_MODE_RTU,
    .rx_timeout = 50000,
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
    static struct modbus_write_coils_msg msg;

    if (zbus_chan_read(&modbus_write_coils_chan, &msg, K_NO_WAIT) != 0) {
        LOG_WRN("Chan read failed or empty");
        return;
    }

    LOG_DBG("Received command: slave_id=%d, start_addr=%d, num_coils=%d", msg.slave_id,
            msg.start_addr, msg.num_coils);
    LOG_HEXDUMP_DBG(msg.values, msg.num_coils, "Coil Values");

    const int write_res = modbus_write_coils(client_iface, msg.slave_id, msg.start_addr,
                                             msg.values, msg.num_coils);
    if (write_res < 0) {
        LOG_ERR("Failed to write coils: %d", write_res);
    }
}

static struct rocket_hydras hydras = {0};

static void rocket_hydra_sample_work_handler(struct k_work *work)
{
    k_work_schedule_for_queue(&modbus_work_q, &rocket_hydra_sample_work,
                              K_MSEC(CONFIG_MODBUS_ROCKET_HYDRA_SAMPLE_INTERVAL));

    rocket_hydras_sensor_read(client_iface, &hydras);

    if (hydras.uf.meta.is_connected) {
        zbus_chan_pub(&uf_hydra_chan,
                      &(const struct uf_hydra_msg){.temperature = hydras.uf.temperature},
                      K_NO_WAIT);
    }

    if (hydras.lf.meta.is_connected) {
        zbus_chan_pub(&lf_hydra_chan,
                      &(const struct lf_hydra_msg){
                          .lf_temperature = hydras.lf.sensors.lf_temperature,
                          .lf_pressure = hydras.lf.sensors.lf_pressure,
                          .cc_pressure = hydras.lf.sensors.cc_pressure,
                      },
                      K_NO_WAIT);
    }
}

void modbus_service_start(void)
{
    rocket_hydras_init(&hydras);

    k_work_queue_start(&modbus_work_q, modbus_work_q_stack,
                       K_THREAD_STACK_SIZEOF(modbus_work_q_stack), CONFIG_MODBUS_WORK_Q_PRIO,
                       NULL);

    k_work_schedule_for_queue(&modbus_work_q, &rocket_hydra_sample_work, K_NO_WAIT);
}
