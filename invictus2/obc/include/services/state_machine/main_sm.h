#ifndef _MAIN_SM_H_
#define _MAIN_SM_H_

#include "zephyr/smf.h"
#include "services/state_machine/main_sm_config.h"
#include "data_models.h"
#include "commands.h"

bool state_machine_service_setup(void);
void state_machine_service_start(void);

/* User defined object */
struct sm_object {
    /* This must be first */
    struct smf_ctx ctx;
    command_t command;
    fill_command_t fill_command;
    system_data_t data;
    struct sm_config *config;
};

extern const struct smf_state states[];

// State Machine Work Helper functions
void toggle_valve(struct sm_object *s, valve_t valve, bool open);
void close_all_valves(struct sm_object *s);
// closes all valves except the one specified
void open_single_valve(struct sm_object *s, valve_t valve);

void sm_init(struct sm_object *initial_s_obj);

#endif // _MAIN_SM_H_
