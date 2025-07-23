#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include "zephyr/modbus/modbus.h"
#include "zephyr/toolchain.h"
#include "zephyr/usb/usb_device.h"

#include "modbus_thrd.h"
#include "filling_sm.h"

LOG_MODULE_REGISTER(modbus_thread, LOG_LEVEL_INF);

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

int modbus_thread_setup(void)
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

    LOG_INF("Master connected on %s", dev->name);
#endif

    const char iface_name[] = {DEVICE_DT_NAME(MODBUS_NODE)};
    client_iface = modbus_iface_get_by_name(iface_name);

    if (client_iface < 0) {
        LOG_ERR("Failed to get client_iface index for %s", iface_name);
        return client_iface;
    }

    return modbus_init_client(client_iface, client_param);
}

void modbus_thread_entry(void *fsm_config, void *data_queues, void *p3)
{
    ARG_UNUSED(p3);

    LOG_INF("Modbus thread started");

    const struct modbus_data_queues *queues = data_queues;

    // TODO: make this configurable.
    // TODO: specify which slave address has which hr addresses and valve coils
    const int slave_addr = 0x01;

    struct filling_sm_object filling_sm_obj = {
        .command = 0,
        .data = {{0}},
        // All valves closed initially.
        // TODO: Making this an union/bitfield would be better.
        .valve_states = 0,
        .config = (struct filling_sm_config *)fsm_config,
    };

    k_timeout_t put_timeout = K_NO_WAIT;
    k_timeout_t get_timeout = K_MSEC(10);

    filling_sm_init(&filling_sm_obj);
    // TODO: Double check loop logic.
    // For now we do
    //  - read sensor data -> put data -> get cmd -> run fsm -> write valve states
    //  - maybe we could also interleave smf run more frequently
    while (1) {
        // TODO: improve loop sleep time calculation
        k_sleep(K_MSEC(1000));

        if (modbus_read_holding_regs(client_iface, slave_addr, 0, filling_sm_obj.data.raw,
                                     ARRAY_SIZE(filling_sm_obj.data.raw)) < 0) {
            LOG_ERR("Failed to read holding registers");
            continue;
        }

        // Put read sensor data into the message queue
        // TODO: handle k_msgq_put() failure
        k_msgq_put(queues->sensor_data_q, &filling_sm_obj.data, put_timeout);

        // Check for commands in the message queue
        // TODO: handle k_msgq_get() failure
        cmd_t cmd = 0;
        k_msgq_get(queues->fsm_cmd_q, &cmd, get_timeout);
        filling_sm_obj.command = cmd;

        smf_run_state(SMF_CTX(&filling_sm_obj));

        if (modbus_write_coils(client_iface, slave_addr, 0, &filling_sm_obj.valve_states, 8)) {
            LOG_ERR("Failed to write valve states");
        }

        LOG_FILLING_DATA(&filling_sm_obj);
        LOG_HEXDUMP_DBG(filling_sm_obj.data.raw, sizeof(filling_sm_obj.data.raw),
                        "Raw holding registers");
    }
}
