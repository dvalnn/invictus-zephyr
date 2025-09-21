#ifndef _FILL_SM_CONFIG_H_
#define _FILL_SM_CONFIG_H_

#include <stdint.h>

// NOTE: Pressures in bar
//       weights in kg
//       temperatures in ºC

#define SAFE_PAUSE_TARGET_N2O_TANK_P  50
#define SAFE_PAUSE_TRIGGER_N2O_TANK_P 52

#define FILL_N2_TARGET_N2_TANK_P  200
#define FILL_N2_TRIGGER_N2_TANK_P 210

#define PRE_PRESS_TARGET_N2O_TANK_P  5
#define PRE_PRESS_TRIGGER_N2O_TANK_P 7

#define FILL_N2O_TARGET_N2O_TANK_P  35
#define FILL_N2O_TARGET_N2O_TANK_W  7
#define FILL_N2O_TRIGGER_N2O_TANK_P 38
#define FILL_N2O_TRIGGER_N2O_TANK_T 2

#define POST_PRESS_TARGET_N2O_TANK_P  50
#define POST_PRESS_TRIGGER_N2O_TANK_P 52

struct filling_sm_config
{
    struct safe_pause
    {
        uint16_t target_n2o_tank_pressure;
        uint16_t trigger_n2o_tank_pressure;
    } safe_pause;

    // State Machine Configuration
    struct fill_n2
    {
        uint16_t target_n2_tank_pressure;
        uint16_t trigger_n2_tank_pressure;
    } fill_n2;

    struct pre_press
    {
        uint16_t target_n2o_tank_pressure;
        uint16_t trigger_n2o_tank_pressure;
    } pre_press;

    struct fill_n2o
    {
        uint16_t target_n2o_tank_weight;
        uint16_t target_n2o_tank_pressure;
        uint16_t trigger_n2o_tank_pressure;
        uint16_t trigger_n2o_tank_temperature; // Minimum temperature safe for the valves
    } fill_n2o;

    struct post_press
    {
        uint16_t target_n2o_tank_pressure;
        uint16_t trigger_n2o_tank_pressure;
    } post_press;
};

#define DEFAULT_FILL_SM_CONFIG(name)                                                          \
    struct filling_sm_config name = {                                                         \
        .safe_pause =                                                                         \
            {                                                                                 \
                .target_n2o_tank_pressure = SAFE_PAUSE_TARGET_N2O_TANK_P,                     \
                .trigger_n2o_tank_pressure = SAFE_PAUSE_TRIGGER_N2O_TANK_P,                   \
            },                                                                                \
        .f_copv =                                                                             \
            {                                                                                 \
                .target_n2_tank_pressure = FILL_N2_TARGET_N2_TANK_P,                          \
                .trigger_n2_tank_pressure = FILL_N2_TRIGGER_N2_TANK_P,                        \
            },                                                                                \
        .pre_p =                                                                              \
            {                                                                                 \
                .target_n2o_tank_pressure = PRE_PRESS_TARGET_N2O_TANK_P,                      \
                .trigger_n2o_tank_pressure = PRE_PRESS_TRIGGER_N2O_TANK_P,                    \
            },                                                                                \
        .f_n2o =                                                                              \
            {                                                                                 \
                .target_n2o_tank_pressure = FILL_N2O_TARGET_N2O_TANK_P,                       \
                .target_n2o_tank_weight = FILL_N2O_TARGET_N2O_TANK_W,                         \
                .trigger_n2o_tank_pressure = FILL_N2O_TRIGGER_N2O_TANK_P,                     \
                .trigger_n2o_tank_temperature = FILL_N2O_TRIGGER_N2O_TANK_T,                  \
            },                                                                                \
        .post_p =                                                                             \
            {                                                                                 \
                .target_n2o_tank_pressure = POST_PRESS_TARGET_N2O_TANK_P,                     \
                .trigger_n2o_tank_pressure = POST_PRESS_TRIGGER_N2O_TANK_P,                   \
            },                                                                                \
    }

#endif // _FILL_SM_CONFIG_H_
