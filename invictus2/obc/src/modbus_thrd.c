#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include "zephyr/modbus/modbus.h"
#include "zephyr/toolchain.h"

#include "modbus_thrd.h"
#include "filling_sm.h"

LOG_MODULE_REGISTER(mbs_sample, LOG_LEVEL_INF);

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

void modbus_thread_entry(void *fsm_config, void *data_queues, void *can_start_sem)
{
    ARG_UNUSED(data_queues);
    k_sem_take((struct k_sem *)can_start_sem, K_FOREVER);

    /* const struct modbus_data_queues *queues = data_queues; */
    if (init_modbus_master()) {
        LOG_ERR("Modbus RTU client initialization failed");
        return;
    }

    LOG_INF("Modbus thread started");
    struct filling_sm_object filling_sm_obj = {
        .command = 0,
        .data = {{0}},
        .valve_states = 0,
        .config = (struct filling_sm_config *)fsm_config,
    };

    filling_sm_init(&filling_sm_obj);

    // TODO: make this configurable.
    // TODO: specify which slave address has which hr addresses and valve coils
    const int slave_addr = 0x01;

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
}
