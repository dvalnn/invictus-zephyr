#include <zephyr/kernel.h>
#include <zephyr/modbus/modbus.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/spi.h>

#include "filling_sm.h"

LOG_MODULE_REGISTER(mbs_sample, LOG_LEVEL_INF);

static int client_iface;

const static struct modbus_iface_param client_param = {
    .mode = MODBUS_MODE_RTU,
    .rx_timeout = 50000,
    .serial =
        {
            // TODO: Double check how high the baud rate can be
            .baud = 115200,
            // NOTE: In RTU mode, modbus uses CRC checks, so parity can be NONE
            .parity = UART_CFG_PARITY_NONE,
            .stop_bits_client = UART_CFG_STOP_BITS_1,
        },
};

#define MODBUS_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_modbus_serial)

static int init_modbus_master(void)
{
    const char iface_name[] = {DEVICE_DT_NAME(MODBUS_NODE)};
    client_iface = modbus_iface_get_by_name(iface_name);

    if (client_iface < 0) {
        LOG_ERR("Failed to get client_iface index for %s", iface_name);
        return client_iface;
    }

    return modbus_init_client(client_iface, client_param);
}

int main(void)
{

#if DT_NODE_HAS_COMPAT(DT_PARENT(MODBUS_NODE), zephyr_cdc_acm_uart)
    const struct device *const dev = DEVICE_DT_GET(DT_PARENT(MODBUS_NODE));
    uint32_t dtr = 0;

    if (!device_is_ready(dev) || usb_enable(NULL)) {
        return 0;
    }

    while (!dtr) {
        uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
        k_sleep(K_MSEC(100));
    }

    LOG_INF("Slave connected on %s", dev->name);
#endif

    if (init_modbus_master()) {
        LOG_ERR("Modbus RTU client initialization failed");
        return 0;
    }

    const int slave_addr = 0x01;

    DEFAULT_FSM_OBJECT(filling_sm_obj, client_iface);
    filling_sm_init(&filling_sm_obj);

    while (1) {
        k_sleep(K_MSEC(1000)); // For now, looping every second is fine

        if (modbus_read_holding_regs(client_iface, slave_addr, 0, filling_sm_obj.data.raw,
                                     ARRAY_SIZE(filling_sm_obj.data.raw)) < 0) {
            LOG_ERR("Failed to read holding registers");
            continue;
        }

        LOG_FILLING_DATA(&filling_sm_obj);

        LOG_HEXDUMP_DBG(filling_sm_obj.data.raw, sizeof(filling_sm_obj.data.raw),
                        "Raw holding registers");
        smf_run_state(SMF_CTX(&filling_sm_obj));
    }

    return 0;
}
