#ifndef _FILLING_SM_H_
#define _FILLING_SM_H_

#include "zephyr/smf.h"

#include "filling_sm_config.h"

#define CMD_OTHER_START  0x00010000
#define CMD_IDLE_START   0x00000100
#define CMD_GLOBAL_START 0x00000001

#define CMD_OTHER_MASK  0x00FF0000
#define CMD_IDLE_MASK   0x0000FF00
#define CMD_GLOBAL_MASK 0x000000FF

#define CMD_OTHER(x)  ((x) & CMD_OTHER_MASK)
#define CMD_IDLE(x)   ((x) & CMD_IDLE_MASK)
#define CMD_GLOBAL(x) ((x) & CMD_GLOBAL_MASK)

enum cmd_global {
    CMD_STOP = CMD_GLOBAL_START, // ANY -> IDLE
    CMD_ABORT,                   // ANY -> ABORT
    CMD_PAUSE,                   // ANY -> SAFE_PAUSE_IDLE
};

enum cmd_idle {
    CMD_FILL_COPV = CMD_IDLE_START, // IDLE -> FILLING_COPV_IDLE
    CMD_PRE_PRESSURIZE,             // IDLE -> PRE_PRESSURIZING_IDLE
    CMD_FILL_N2O,                   // IDLE -> FILLING_N2O_IDLE
    CMD_POST_PRESSURIZE,            // IDLE -> POST_PRESSURIZING_IDLE
};

enum cmd_other {
    CMD_READY = CMD_OTHER_START, // ABORT -> IDLE
    CMD_RESUME,                  // SAFE_PAUSE -> IDLE
};

typedef uint32_t cmd_t;

/* List of possible states */
enum filling_state {
    ROOT, // parent state to all states. Used to handle global commands

    IDLE,
    ABORT,
    MANUAL_OP, // NOTE: Acept manual override commands from the ground station

    // SAFE PAUSE
    SAFE_PAUSE, // parent state
    SAFE_PAUSE_IDLE,
    SAFE_PAUSE_VENT,

    // FILLING_COPV (filling N)
    FILLING_COPV, // parent state
    FILLING_COPV_IDLE,
    FILLING_COPV_FILL,

    // PRE_PRESSURIZING
    PRE_PRESSURIZING, // parent state
    PRE_PRESSURIZING_IDLE,
    PRE_PRESSURIZING_VENT,
    PRE_PRESSURIZING_FILL_N,

    // FILLING_N2O
    FILLING_N2O, // parent state
    FILLING_N2O_IDLE,
    FILLING_N2O_FILL,
    FILLING_N2O_VENT,

    // POST_PRESSURIZING
    POST_PRESSURIZING, // parent state
    POST_PRESSURIZING_IDLE,
    POST_PRESSURIZING_VENT,
    POST_PRESSURIZING_FILL_N,
};

enum valves {
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
    VALVE_COUNT,
};

struct filling_sm_config {
    struct safe_pause {
        uint16_t target_n2o_tank_pressure; 
        uint16_t trigger_n2o_tank_pressure;
    } safe_pause;

    // State Machine Configuration
    struct filling_copv {
        uint16_t target_n2_tank_pressure; // NOTE: this is the pressure on the n2 line before the tank, as there is no pressure sensor on the n2 tank.
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
struct filling_sm_object {
    /* This must be first */
    struct smf_ctx ctx;

    cmd_t command;
    union filling_data data;

    // TODO: Make this into an union
    uint16_t valve_states;

    struct filling_sm_config *config;
};

#ifdef UNIT_TEST
extern const struct smf_state filling_states[];
#endif

void filling_sm_init(struct filling_sm_object *initial_s_obj);

#define LOG_FILLING_DATA(fsm_data)                                                            \
    do {                                                                                     \
        LOG_INF("N2 Tank P: %u", (fsm_data).n2_tank_pressure);                             \
        LOG_INF("N2O Tank P: %u", (fsm_data).n2o_tank_pressure);                          \
        LOG_INF("N2O Tank W: %u", (fsm_data).n2o_tank_weight);                            \
        LOG_INF("N2O Tank T: %u", (fsm_data).n2o_tank_temperature);                       \
    } while (0)

#define DEFAULT_FSM_CONFIG(name)                                                              \
    struct filling_sm_config name = {                                                         \
        .safe_pause =                                                                         \
            {                                                                                 \
                .target_n2o_tank_pressure = SAFE_PAUSE_TARGET_N2O_TANK_P,                          \
                .trigger_n2o_tank_pressure = SAFE_PAUSE_TRIGGER_N2O_TANK_P,                        \
            },                                                                                \
        .f_copv =                                                                             \
            {                                                                                 \
                .target_n2_tank_pressure = FILLING_COPV_TARGET_N2_TANK_P,                        \
            },                                                                                \
        .pre_p =                                                                              \
            {                                                                                 \
                .target_n2o_tank_pressure = PRE_PRESSURIZING_TARGET_N2O_TANK_P,                       \
                .trigger_n2o_tank_pressure = PRE_PRESSURIZING_TRIGGER_N2O_TANK_P,                     \
            },                                                                                \
        .f_n2o =                                                                              \
            {                                                                                 \
                .target_n2o_tank_pressure = FILLING_N2O_TARGET_N2O_TANK_P,                            \
                .target_n2o_tank_weight = FILLING_N2O_TARGET_N2O_TANK_W,                              \
                .trigger_n2o_tank_pressure = FILLING_N2O_TRIGGER_N2O_TANK_P,                          \
                .trigger_n2o_tank_temperature = FILLING_N2O_TRIGGER_N2O_TANK_T,                              \
            },                                                                                \
        .post_p =                                                                             \
            {                                                                                 \
                .target_n2o_tank_pressure = POST_PRESSURIZING_TARGET_N2O_TANK_P,                      \
                .trigger_n2o_tank_pressure = POST_PRESSURIZING_TRIGGER_N2O_TANK_P,                    \
            },                                                                                \
    }

#endif // _FILLING_SM_H_
