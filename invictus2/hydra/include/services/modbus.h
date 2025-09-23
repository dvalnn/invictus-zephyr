#ifndef MODBUS_H
#define MODBUS_H

#include <zephyr/kernel.h>
#include <zephyr/modbus/modbus.h>
#include <zephyr/logging/log.h>


typedef enum {
    SOL_VALVE_1 = 0,
    SOL_VALVE_2,
    SOL_VALVE_3,
    SB_VALVE_1,
    SB_VALVE_2,
    QDC_VALVE_1,
    QDC_VALVE_2,
    SERVO_1,
    ACTUATOR_COUNT
} actuators_t;

typedef enum {
    THERMO_1 = 0,
    THERMO_2,
    THERMO_3,
    PRESSURE_1,
    PRESSURE_2,
    PRESSURE_3,
    SENSOR_COUNT
} sensors_t;

typedef struct {
    /* Note that the array that will be receiving the coil
     * values must be greater than or equal to:
     *                   (num_coils - 1) / 8 + 1
     */
    uint8_t coils[ACTUATOR_COUNT / 8 + 2]; // +2 to be safe since division is rounded down
    uint16_t holding_registers[SENSOR_COUNT]; // each sensor uses one register
} modbus_memory_t;

int init_modbus_server(void);

#endif // MODBUS_H