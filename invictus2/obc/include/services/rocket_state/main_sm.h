#ifndef _MAIN_SM_H_
#define _MAIN_SM_H_

#include "zephyr/smf.h"
#include "main_sm_config.h"
#include "data_models.h"
#include "commands.h"

enum valves {
    _VALVE_NONE = 0,

    VALVE_N2O_FILL,
    VALVE_N2O_PURGE,
    VALVE_N_FILL,
    VALVE_N_PURGE,
    VALVE_N2O_QUICK_DC,
    VALVE_N2_QUICK_DC,
    VALVE_PRESSURIZING,
    VALVE_MAIN,
    VALVE_VENT,
    VALVE_ABORT,

    _VALVE_COUNT,
};

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

union filling_data {
    struct {
        uint16_t n2_tank_pressure; // This value is from the N2 line (Filling Station)
        uint16_t n2o_tank_pressure;
        uint16_t n2o_tank_weight;
        uint16_t n2o_tank_temperature; // Temperature of the N2O tank
    }; // anonymous struct
    uint16_t raw[4];
};

/* User defined object */
struct sm_object {
    /* This must be first */
    struct smf_ctx ctx;

    command_t command;

    struct full_system_data_s data;

    struct sm_config *config;
};

#ifdef UNIT_TEST
extern const struct smf_state states[];
#endif

void sm_init(struct sm_object *initial_s_obj);

#define DEFAULT_FSM_CONFIG(name)                                                              \
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

#endif // _FILLING_SM_H_
