#ifndef MODBUS_H
#define MODBUS_H

#include <zephyr/kernel.h>
#include <zephyr/modbus/modbus.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(modbus, LOG_LEVEL_INF);

typedef enum {
    sol_valve_1,
    sol_valve_2,
    sol_valve_3,
    sb_valve_1,
    sb_valve_2,
    qdc_valve_1,
    qdc_valve_2,
    servo_1,
} actuators_t;

typedef enum {
    thermo_1,
    thermo_2,
    thermo_3,
    pressure_1,
    pressure_2,
    pressure_3,
} sensors_t;

#endif // MODBUS_H