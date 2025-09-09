#include "services/state_machine/main_sm.h"
#include "services/state_machine/filling_sm.h"
#include "services/state_machine/flight_sm.h"

#include "data_models.h"
#include "zephyr/smf.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main_sm, LOG_LEVEL_DBG);

#ifndef UNIT_TEST
/* Forward declaration of state table.
 * If unittesting, the state table is exported from the header file instead.
 */
static const struct smf_state states[];
#endif

static void root_entry(void *o)
{
    ARG_UNUSED(o);
    LOG_DBG("Entered ROOT state");
}

static void root_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    LOG_DBG("Running ROOT state");

    command_t cmd = s->command;
    if (!cmd) {
        return;
    }

    switch (cmd) {
        case CMD_STOP:
            LOG_DBG("Global transition: CMD_STOP -> IDLE");
            smf_set_state(SMF_CTX(s), &states[SM_IDLE]);
            break;

        case CMD_ABORT:
            LOG_DBG("Global transition: CMD_ABORT -> ABORT");
            smf_set_state(SMF_CTX(s), &states[SM_ABORT]);
            break;
        default:
            LOG_ERR("Unknown global command: %d", cmd);
    }

    cmd = 0; // Clear command
}

root_exit(void *o)
{
    ARG_UNUSED(o);
    LOG_DBG("Exiting ROOT state");
}


/* State Callbacks */

static void idle_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    LOG_DBG("Entered IDLE state");

    close_all_valves(s);
}

static void idle_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    LOG_DBG("Running IDLE state");

    command_t cmd = s->command;
    if (!cmd) {
        return;
    }

    switch (cmd) {
        case CMD_FILL_EXEC:
            LOG_DBG("IDLE: FILL_EXEC -> FILL");
            smf_set_state(SMF_CTX(s), &states[SM_FILL]);
            break;
        case CMD_READY:
            smf_set_state(SMF_CTX(s), &states[SM_READY]);
            break;
        default:
            LOG_ERR("Can't transition from idle to: %d", cmd);
            smf_set_handled(SMF_CTX(s));
    }

    cmd = 0; // Clear command
}

static void idle_exit(void *o)
{
    ARG_UNUSED(o);
    LOG_DBG("Exiting IDLE state");
}

static void filling_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    LOG_DBG("Entered FILL state");
    // Prepare for filling sequence, e.g., ensure safe valve states
    close_all_valves(s);
}

static void filling_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    LOG_DBG("Running RS_FILLING state");

    filling_command_t cmd = s->fill_command;
    if (!cmd) {
        LOG_WRN("No filling command");
        return;
    }

    switch (cmd) {
        case CMD_FILL_N2:
            LOG_DBG("RS_FILLING: CMD_FILL_N2 -> RS_FILL_FILL_N2_FILL");
            smf_set_state(SMF_CTX(s), &states[SM_FILL_FILL_N2]);
            break;
        case CMD_FILL_PRE_PRESS:
            LOG_DBG("RS_FILLING: CMD_FILL_PRE_PRESS -> RS_FILL_FILL_PRE_PRESS");
            smf_set_state(SMF_CTX(s), &states[SM_FILL_PRE_PRESS]);
            break;
        case CMD_FILL_N2O:
            LOG_DBG("RS_FILLING: CMD_FILL_N2O -> RS_FILL_FILL_N2O");
            smf_set_state(SMF_CTX(s), &states[SM_FILL_FILL_N2O]);
            break;
        case CMD_FILL_POST_PRESS:
            LOG_DBG("RS_FILLING: CMD_FILL_POST_PRESS -> RS_FILL_POST_PRESS");
            smf_set_state(SMF_CTX(s), &states[SM_FILL_POST_PRESS]);
            break;
        default:
            LOG_ERR("Unknown filling command in RS_FILLING: %d", cmd);
            smf_set_handled(SMF_CTX(s));
    }

    cmd = 0; // Clear command
}

static void filling_exit(void *o)
{
    ARG_UNUSED(o);
    LOG_DBG("Exiting RS_FILLING state");
}

static void ready_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    LOG_DBG("Entered RS_READY state");
    // Prepare system for arming, e.g., verify sensors
}

static void ready_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    LOG_DBG("Running RS_READY state");

    command_t cmd = s->command;
    if (!cmd) {
        return;
    }

    switch (cmd) {
        case CMD_ARM:
            LOG_DBG("RS_READY: CMD_ARM -> RS_ARMED");
            smf_set_state(SMF_CTX(s), &states[RS_ARMED]);
            break;
        case CMD_ABORT:
            LOG_DBG("RS_READY: CMD_ABORT -> RS_ABORT");
            smf_set_state(SMF_CTX(s), &states[RS_ABORT]);
            break;
        default:
            LOG_ERR("Unknown command in RS_READY: %d", cmd);
            smf_set_handled(SMF_CTX(s));
    }

    cmd = 0; // Clear command
}

static void ready_exit(void *o)
{
    ARG_UNUSED(o);
    LOG_DBG("Exiting RS_READY state");
}

static void armed_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    LOG_DBG("Entered RS_ARMED state");
    // System is armed, prepare for launch
}

static void armed_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    LOG_DBG("Running RS_ARMED state");

    command_t cmd = s->command;
    if (!cmd) {
        return;
    }

    switch (cmd) {
        case CMD_FIRE:
            LOG_DBG("RS_ARMED: CMD_FIRE -> RS_FLIGHT");
            if(s->data.thermocouples.chamber_thermo > s->config->flight_sm_config.min_chamber_launch_temp){
                LOG_DBG("Chamber temperature is above minimum, proceeding to FLIGHT");
            } else {
                LOG_ERR("Chamber temperature too low for launch: %d", s->data.thermocouples.chamber_thermo);
                smf_set_handled(SMF_CTX(s));
                break;
            }
            smf_set_state(SMF_CTX(s), &states[RS_FLIGHT]);
            break;
        default:
            LOG_ERR("Unknown command in RS_ARMED: %d", cmd);
            smf_set_handled(SMF_CTX(s));
    }

    cmd = 0; // Clear command
}

static void armed_exit(void *o)
{
    ARG_UNUSED(o);
    LOG_DBG("Exiting RS_ARMED state");
}

static void flight_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    LOG_DBG("Entered FLIGHT state");
}

static void flight_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    LOG_DBG("Running FLIGHT state");

    // handle in flight sub-state machine

    cmd = 0; // Clear command
}

static void flight_exit(void *o)
{
    ARG_UNUSED(o);
    LOG_DBG("Exiting FLIGHT state");
}



static void abort_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    (void)s; // unused
    LOG_DBG("Entered ABORT state");
    close_all_valves(s);
    toggle_valve(s, VALVE_ABORT, true);
    // TODO: wait x seconds for main tank to purge
    toggle_valve(s, VALVE_PRESSURIZING, true);
}

static void abort_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    LOG_DBG("Running ABORT state");

    enum cmd_other cmd = CMD_OTHER(s->command);
    if (!cmd) {
        return;
    }

    switch (cmd) {
    case CMD_READY:
        LOG_DBG("ABORT state: CMD_READY -> IDLE");
        smf_set_state(SMF_CTX(s), &states[RS_IDLE]);
        break;
    case CMD_STOP:
        LOG_DBG("ABORT state: CMD_STOP -> IDLE");
        smf_set_state(SMF_CTX(s), &states[RS_IDLE]);
        break;
    default:
        LOG_ERR("Unknown OTHER command: %d", cmd);
        smf_set_handled(SMF_CTX(s));
    }

    cmd = 0; // Clear command
}

static void abort_exit(void *o)
{
    ARG_UNUSED(o);
    LOG_DBG("Exiting RS_FILL_ABORT state");
}


#ifdef UNIT_TEST
const struct smf_state states[] = {
#else
static const struct smf_state states[] = {
#endif
    // clang-format off
    // SMF_CREATE_STATE(s_entry_cb, s_run_cb, s_exit_cb, s_parent, s_initial_o),
    [SM_ROOT]      = SMF_CREATE_STATE(NULL, root_run, NULL, NULL, &states[SM_IDLE]),

    [SM_IDLE]      = SMF_CREATE_STATE(idle_entry, idle_run, idle_exit, &states[SM_ROOT], NULL),
    [SM_ABORT]     = SMF_CREATE_STATE(abort_entry, abort_run, abort_exit,&states[SM_ROOT], NULL),

    [SM_FILL]   = SMF_CREATE_STATE(filling_entry, filling_run, filling_exit, &states[SM_ROOT], NULL),
    
    // filling sub-states
    [SM_FILL_SAFE_PAUSE] = SMF_CREATE_STATE(filling_safe_pause_entry, filling_safe_pause_run, filling_safe_pause_exit, &states[SM_FILL], NULL),
    [SM_FILL_FILL_N2] = SMF_CREATE_STATE(filling_fill_n2_entry, filling_fill_n2_run, filling_fill_n2_exit, &states[SM_FILL], NULL),
    [SM_FILL_PRE_PRESS] = SMF_CREATE_STATE(filling_pre_press_entry, filling_pre_press_run, filling_pre_press_exit, &states[SM_FILL], NULL),
    [SM_FILL_FILL_N2O] = SMF_CREATE_STATE(filling_fill_n2o_entry, filling_fill_n2o_run, filling_fill_n2o_exit, &states[SM_FILL], NULL),
    [SM_FILL_POST_PRESS] = SMF_CREATE_STATE(filling_post_press_entry, filling_post_press_run, filling_post_press_exit, &states[SM_FILL], NULL),

    [SM_READY]      = SMF_CREATE_STATE(ready_entry, ready_run, ready_exit, &states[SM_ROOT], NULL),
    [SM_ARMED]      = SMF_CREATE_STATE(armed_entry, armed_run, armed_exit, &states[SM_ROOT], NULL),
    [SM_FLIGHT]     = SMF_CREATE_STATE(flight_entry, flight_run, flight_exit, &states[SM_ROOT], NULL),

    [SM_FLIGHT]     = SMF_CREATE_STATE(flight_entry, flight_run, flight_exit, &states[SM_FLIGHT], NULL),
    // clang-format on
};

void sm_init(struct sm_object *initial_s_obj)
{
    LOG_DBG("Initializing state machine: setting initial state to RS_IDLE");
    smf_set_initial(SMF_CTX(initial_s_obj), &states[RS_IDLE]);
}
