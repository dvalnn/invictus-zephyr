#ifndef _FILLING_SM_CONFIG_H_
#define _FILLING_SM_CONFIG_H_

#include <stdint.h>

// NOTE: Pressures in bar
//       weights in kg
//       temperatures in ºC

#define SAFE_PAUSE_TARGET_N2O_TANK_P  50
#define SAFE_PAUSE_TRIGGER_N2O_TANK_P 52

#define FILLING_COPV_TARGET_N2_TANK_P 200

#define PRE_PRESSURIZING_TARGET_N2O_TANK_P  5
#define PRE_PRESSURIZING_TRIGGER_N2O_TANK_P 7

#define FILLING_N2O_TARGET_N2O_TANK_P  35
#define FILLING_N2O_TARGET_N2O_TANK_W  7
#define FILLING_N2O_TRIGGER_N2O_TANK_P 38
#define FILLING_N2O_TRIGGER_N2O_TANK_T 2

#define POST_PRESSURIZING_TARGET_N2O_TANK_P  50
#define POST_PRESSURIZING_TRIGGER_N2O_TANK_P 52

struct filling_sm_config {
    struct safe_pause {
        uint16_t target_n2o_tank_pressure;
        uint16_t trigger_n2o_tank_pressure;
    } safe_pause;

    // State Machine Configuration
    struct filling_copv {
        uint16_t
            target_n2_tank_pressure; // NOTE: this is the pressure on the n2 line before the
                                     // tank, as there is no pressure sensor on the n2 tank.
    } f_copv;

    struct pre_pressurizing {
        uint16_t target_n2o_tank_pressure;
        uint16_t trigger_n2o_tank_pressure;
    } pre_p;

    struct filling_n2o {
        uint16_t target_n2o_tank_weight;
        uint16_t target_n2o_tank_pressure;
        uint16_t trigger_n2o_tank_pressure;
        uint16_t trigger_n2o_tank_temperature; // Minimum temperature safe for the valves
    } f_n2o;

    struct post_pressurizing {
        uint16_t target_n2o_tank_pressure;
        uint16_t trigger_n2o_tank_pressure;
    } post_p;
};

#define DEFAULT_FILLING_SM_CONFIG(name)                                                              \
    struct filling_sm_config name = {                                                         \
        .safe_pause =                                                                         \
            {                                                                                 \
                .target_n2o_tank_pressure = SAFE_PAUSE_TARGET_N2O_TANK_P,                     \
                .trigger_n2o_tank_pressure = SAFE_PAUSE_TRIGGER_N2O_TANK_P,                   \
            },                                                                                \
        .f_copv =                                                                             \
            {                                                                                 \
                .target_n2_tank_pressure = FILLING_COPV_TARGET_N2_TANK_P,                     \
            },                                                                                \
        .pre_p =                                                                              \
            {                                                                                 \
                .target_n2o_tank_pressure = PRE_PRESSURIZING_TARGET_N2O_TANK_P,               \
                .trigger_n2o_tank_pressure = PRE_PRESSURIZING_TRIGGER_N2O_TANK_P,             \
            },                                                                                \
        .f_n2o =                                                                              \
            {                                                                                 \
                .target_n2o_tank_pressure = FILLING_N2O_TARGET_N2O_TANK_P,                    \
                .target_n2o_tank_weight = FILLING_N2O_TARGET_N2O_TANK_W,                      \
                .trigger_n2o_tank_pressure = FILLING_N2O_TRIGGER_N2O_TANK_P,                  \
                .trigger_n2o_tank_temperature = FILLING_N2O_TRIGGER_N2O_TANK_T,               \
            },                                                                                \
        .post_p =                                                                             \
            {                                                                                 \
                .target_n2o_tank_pressure = POST_PRESSURIZING_TARGET_N2O_TANK_P,              \
                .trigger_n2o_tank_pressure = POST_PRESSURIZING_TRIGGER_N2O_TANK_P,            \
            },                                                                                \
    }

#endif // _FILLING_SM_CONFIG_H_