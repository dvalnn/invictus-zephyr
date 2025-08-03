#include "threads/modbus.h"

#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include "zephyr/modbus/modbus.h"
#include "zephyr/usb/usb_device.h"
#include "zephyr/zbus/zbus.h"

#include "hydra.h"
#include "zbus_messages.h"

LOG_MODULE_REGISTER(obc_modbus, LOG_LEVEL_DBG);

ZBUS_OBS_DECLARE(modbus_coil_write_obs)
ZBUS_CHAN_DECLARE(modbus_coil_write_chan, uf_hydra_chan, lf_hydra_chan, fs_hydra_chan);

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

#define MODBUS_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_modbus_serial)

bool modbus_thread_setup(void)
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

void sample_and_pub_hydras(struct hydras *hydras, const k_timeout_t pub_timeout)
{
    // Read hydras data
    const int hydra_read_result = hydras_modbus_read(hydras, client_iface);
    if (hydra_read_result < 0) {
        LOG_ERR("Failed to read hydras data: %d", hydra_read_result);
        return;
    }

    LOG_DBG("UF Temperature %d", hydras->uf.temperature);
    LOG_HEXDUMP_DBG(hydras->lf.sensors.raw, sizeof(hydras->lf.sensors.raw),
                    "LF Sensors Raw Data");

    const struct uf_hydra_msg uf_msg = {
        .temperature = hydras->uf.temperature,
    };

    const struct lf_hydra_msg lf_msg = {
        .lf_temperature = hydras->lf.sensors.lf_temperature,
        .lf_pressure = hydras->lf.sensors.lf_pressure,
        .cc_pressure = hydras->lf.sensors.cc_pressure,
    };

    zbus_chan_pub(&uf_hydra_chan, &uf_msg, pub_timeout);
    zbus_chan_pub(&lf_hydra_chan, &lf_msg, pub_timeout);
}

void handle_write_coils_msg(const k_timeout_t sub_timeout, const k_timeout_t read_timeout)
{
    const static struct zbus_channel *chan;
    static struct modbus_write_coils_msg write_msg = {0};

    // Get command from ZBUS
    zbus_sub_wait(&modbus_coil_write_obs, &chan, sub_timeout);
    if (chan != &modbus_coil_write_chan) {
        LOG_DBG("No command received, skipping");
        return;
    }

    // Get the command message
    zbus_chan_read(&modbus_coil_write_chan, &write_msg, read_timeout);
    LOG_DBG("Received command: slave_id=%d, start_addr=%d, num_coils=%d", write_msg.slave_id,
            write_msg.start_addr, write_msg.num_coils);
    LOG_HEXDUMP_DBG(write_msg.values, write_msg.num_coils, "Coil Values");

    const int write_res =
        modbus_write_coils(client_iface, write_msg.slave_id, write_msg.start_addr,
                           write_msg.values, write_msg.num_coils);
    if (write_res < 0) {
        LOG_ERR("Failed to write coils: %d", write_res);
    }
}

void modbus_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    LOG_INF("Modbus thread started");

    const k_timeout_t pub_timeout = K_NO_WAIT;
    const k_timeout_t sub_timeout = K_NO_WAIT;
    const k_timeout_t read_timeout = K_NO_WAIT;

    struct hydras hydras = {0};
    hydras_init(&hydras);

    // TODO: Double check loop logic.
    // For now we do
    //  - read sensor data -> put data -> get cmd -> run fsm -> write valve states
    //  - maybe we could also interleave smf run more frequently
    while (1) {
        // TODO: improve loop sleep time calculation
        k_sleep(K_MSEC(500));

        sample_and_pub_hydras(&hydras, pub_timeout);
        handle_write_coils_msg(sub_timeout, read_timeout);
    }
}
