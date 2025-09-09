#ifndef _MAIN_SM_H_
#define _MAIN_SM_H_

#include "zephyr/smf.h"
#include "main_sm_config.h"
#include "data_models.h"
#include "commands.h"

typedef enum sm_state_e { // state machine's internal state
    SM_ROOT,
    SM_IDLE,
    SM_FILL,

    // Filling Substates
    SM_FILL_SAFE_PAUSE,
    SM_FILL_SAFE_PAUSE_IDLE,
    SM_FILL_SAFE_PAUSE_VENT,

    SM_FILL_FILL_N2,
    SM_FILL_FILL_N2_IDLE,
    SM_FILL_FILL_N2_FILL, // 3 fills in one state, maybe not very good :/

    SM_FILL_PRE_PRESS,
    SM_FILL_PRE_PRESS_IDLE,
    SM_FILL_PRE_PRESS_FILL_N2,
    SM_FILL_PRE_PRESS_VENT,

    SM_FILL_FILL_N2O,
    SM_FILL_FILL_N2O_IDLE,
    SM_FILL_FILL_N2O_FILL,
    SM_FILL_FILL_N2O_VENT,

    SM_FILL_POST_PRESS,
    SM_FILL_POST_PRESS_IDLE,
    SM_FILL_POST_PRESS_FILL_N2,
    SM_FILL_POST_PRESS_VENT,

    SM_READY,
    SM_ARMED,
    SM_FLIGHT,

    // Flight Substates
    SM_FLIGHT_LAUNCH,
    SM_FLIGHT_ASCENT,
    SM_FLIGHT_APOGEE,
    SM_FLIGHT_DROGUE_CHUTE,
    SM_FLIGHT_MAIN_CHUTE,
    SM_FLIGHT_TOUCHDOWN,

    SM_ABORT,
} sm_state_t;

bool state_machine_service_setup(void);
void state_machine_service_start(void);

/* User defined object */
struct sm_object {
    /* This must be first */
    struct smf_ctx ctx;
    command_t command;
    fill_command_t fill_command;
    struct full_system_data_s data;
    struct sm_config *config;
};

#ifdef UNIT_TEST
extern const struct smf_state states[];
#endif

void sm_init(struct sm_object *initial_s_obj);

// State Machine Work Helper functions
void valve_toggle(struct sm_object *s, enum valves valve, bool state);
void close_all_valves(struct sm_object *s);
// closes all valves except the one specified
void open_single_valve(struct sm_object *s, enum valves valve);

#endif // _FILLING_SM_H_
