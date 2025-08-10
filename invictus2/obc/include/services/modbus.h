#ifndef MODBUS_THRD_H_
#define MODBUS_THRD_H_

#include <stdbool.h>
#include <stdint.h>

struct modbus_slave_metadata {
    uint8_t slave_id;     // Modbus slave ID
    uint8_t is_connected; // Connection status
    uint16_t ir_start;    // Start address for input registers
};

bool modbus_service_setup(void);
void modbus_service_start(void);

void modbus_slave_check_connection(const int rc, struct modbus_slave_metadata *const meta,
                                   const char *const label);
#endif
