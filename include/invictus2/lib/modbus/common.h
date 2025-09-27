#ifndef MODBUS_REGISTERS_H_
#define MODBUS_REGISTERS_H_

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/modbus/modbus.h>

struct modbus_register_meta
{
    uint16_t start_addr;
    uint16_t count;
};

struct modbus_server_meta
{
    int unit_id;

    struct modbus_register_meta coils;
    struct modbus_register_meta discrete_inputs;
    struct modbus_register_meta holding_registers;
    struct modbus_register_meta input_registers;

#if defined(CONFIG_MODBUS_FP_EXTENSIONS)
    struct modbus_register_meta input_registers_fp;
    struct modbus_register_meta holding_registers_fp;
#endif
};

struct modbus_server_registers
{
    uint8_t *coils;
    uint8_t *discrete_inputs;
    uint16_t *holding_registers;
    uint16_t *input_registers;

#if defined(CONFIG_MODBUS_FP_EXTENSIONS)
    float *input_registers_fp;
    float *holding_registers_fp;
#endif
};

struct modbus_server
{
    struct modbus_server_meta meta;
    struct modbus_server_registers regs;
    struct k_mutex regs_mutex;
};

int modbus_server_init(struct modbus_server *server, struct modbus_server_meta meta);
int modbus_server_free(struct modbus_server *server);

int modbus_server_start(const struct modbus_server *const server, int modbus_iface,
                        struct modbus_serial_param serial_param);

int modbus_server_stop(const struct modbus_server *const server);

static inline int modbus_server_reg_lock(struct modbus_server *const server,
                                         k_timeout_t timeout)
{
    return k_mutex_lock(&server->regs_mutex, timeout);
}

static inline int modbus_server_reg_unlock(struct modbus_server *const server)
{
    return k_mutex_unlock(&server->regs_mutex);
}

#endif // MODBUS_REGISTERS_H_
