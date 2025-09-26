#ifndef DATA_MODELS_H
#define DATA_MODELS_H

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
    VALVE_COUNT
} valve_t;

typedef enum {
    THERMO_1 = 0,
    THERMO_2,
    THERMO_3,
    PRESSURE_1,
    PRESSURE_2,
    PRESSURE_3,
    SENSOR_COUNT
} sensor_t;

typedef struct {
    uint8_t valve_states[VALVE_COUNT];
} valves_msg_t;

typedef struct {
    int16_t thermo1, thermo2, thermo3;
} temps_msg_t;

typedef struct
{
    uint16_t pressure1, pressure2, pressure3;
} press_msg_t;

typedef struct {
    uint8_t valve_states[VALVE_COUNT];
    uint16_t sensor_values[SENSOR_COUNT];
} hydra_data_t;

#endif // DATA_MODELS_H