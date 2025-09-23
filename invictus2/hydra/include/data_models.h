#ifndef DATA_MODELS_H
#define DATA_MODELS_H

#include <zephyr/kernel.h>
#include <zephyr/modbus/modbus.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(data_models, LOG_LEVEL_INF);

typedef struct {
    uint8_t valve_states[ACTUATOR_COUNT];
    uint16_t sensor_values[SENSOR_COUNT];
} data_model_t;

#endif // DATA_MODELS_H