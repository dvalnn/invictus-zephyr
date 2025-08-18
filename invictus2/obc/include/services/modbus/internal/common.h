#ifndef MODBUS_COMMON_H
#define MODBUS_COMMON_H

#include <stdint.h>

struct modbus_slave_metadata {
    uint8_t slave_id;     // Modbus slave ID
    uint8_t is_connected; // Connection status
    uint16_t ir_start;    // Start address for input registers
};

void modbus_slave_check_connection(const int read_result,
                                   struct modbus_slave_metadata *const meta,
                                   const char *const label);

#endif // MODBUS_COMMON_H
