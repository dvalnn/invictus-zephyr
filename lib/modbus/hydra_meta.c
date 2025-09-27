#include "invictus2/lib/modbus/common.h"

const struct modbus_server_meta hydra_uf_meta = {
    .unit_id = CONFIG_MODBUS_HYDRA_UF_SLAVE_ID,
    .coils =
        {
            .start_addr = CONFIG_MODBUS_HYDRA_UF_COILS_ADDR_START,
            .count = CONFIG_MODBUS_HYDRA_UF_COILS_COUNT,
        },
    .input_registers =
        {
            .start_addr = CONFIG_MODBUS_HYDRA_UF_INPUT_REGS_ADDR_START,
            .count = CONFIG_MODBUS_HYDRA_UF_INPUT_REGS_COUNT,
        },
    .discrete_inputs = {},
    .holding_registers = {},

    .input_registers_fp = {},
    .holding_registers_fp = {},
};
