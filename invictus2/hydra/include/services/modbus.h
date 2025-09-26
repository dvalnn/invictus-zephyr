#ifndef MODBUS_H
#define MODBUS_H

#include <zephyr/kernel.h>
#include <zephyr/modbus/modbus.h>
#include <zephyr/logging/log.h>
#include "data_models.h"

typedef struct {
    /* Note that the array that will be receiving the coil
     * values must be greater than or equal to:
     *                   (num_coils - 1) / 8 + 1
     */
    uint8_t coils[(VALVE_COUNT - 1) / 8 + 1];
    uint16_t holding_registers[SENSOR_COUNT]; // each sensor uses one register
} modbus_memory_t;

int modbus_setup(void);
void modbus_start(void);

#endif // MODBUS_H