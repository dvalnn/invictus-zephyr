#include "services/state_machine/main_sm.h"
#include "services/state_machine/filling_sm.h"
#include "services/state_machine/flight_sm.h"

#include "data_models.h"

#include <zephyr/logging/log.h>
#include <zephyr/smf.h>
#include <stdbool.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(state_machine_service, LOG_LEVEL_DBG);

/* ===================================================================== */
/* Helper Functions                                                      */
/* ===================================================================== */

void set_valve(struct sm_object *s, valve_t valve, bool open)
{
    switch (valve)
    {
    case VALVE_N2O_FILL:
        s->data.actuators.v_n2o_fill = open ? 1 : 0;
        break;
    case VALVE_N2O_PURGE:
        s->data.actuators.v_n2o_purge = open ? 1 : 0;
        break;
    case VALVE_N2_FILL:
        s->data.actuators.v_n2_fill = open ? 1 : 0;
        break;
    case VALVE_N2_PURGE:
        s->data.actuators.v_n2_purge = open ? 1 : 0;
        break;
    case VALVE_N2O_QUICK_DC:
        s->data.actuators.v_n2o_quick_dc = open ? 1 : 0;
        break;
    case VALVE_N2_QUICK_DC:
        s->data.actuators.v_n2_quick_dc = open ? 1 : 0;
        break;
    case VALVE_PRESSURIZING:
        s->data.actuators.v_pressurizing = open ? 1 : 0;
        break;
    case VALVE_MAIN:
        s->data.actuators.v_main = open ? 1 : 0;
        break;
    case VALVE_VENT:
        s->data.actuators.v_vent = open ? 1 : 0;
        break;
    case VALVE_ABORT:
        s->data.actuators.v_abort = open ? 1 : 0;
        break;
    default:
        break;
    }
}

inline void close_all_valves(struct sm_object *s)
{
    s->data.actuators.rocket_valves_mask = 0;
    s->data.actuators.fill_station_valves_mask = 0;
    s->data.actuators.quick_dc_mask = 0;
}

// closes all valves except the one specified
inline void open_single_valve(struct sm_object *s, valve_t valve)
{
    close_all_valves(s);
    set_valve(s, valve, true);
}

/* ===================================================================== */
/* root: Top level state. Used to check for global commands.             */
/* ===================================================================== */

static void root_entry(void *o)
{
    ARG_UNUSED(o);
}

static void root_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;

    command_t cmd = s->command;
    if (!cmd)
    {
        return;
    }

    switch (cmd)
    {
    case CMD_STOP:
        smf_set_state(SMF_CTX(s), &states[IDLE]);
        break;

    case CMD_ABORT:
        smf_set_state(SMF_CTX(s), &states[ABORT]);
        break;
    default:
        // Unknown global command
        ;
    }

    cmd = 0;
}

static void root_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ===================================================================== */
/* root/idle: System is idle. All valves closed.                         */
/* ===================================================================== */

static void idle_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}

static void idle_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;

    command_t cmd = s->command;
    if (!cmd)
    {
        return;
    }

    switch (cmd)
    {
    case CMD_FILL_EXEC:
        smf_set_state(SMF_CTX(s), &states[FILL]);
        break;
    case CMD_READY:
        smf_set_state(SMF_CTX(s), &states[READY]);
        break;
    default:
        smf_set_handled(SMF_CTX(s));
    }

    cmd = 0;
}

static void idle_exit(void *o)
{
    ARG_UNUSED(o);
}

static void fill_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}

static void fill_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;

    fill_command_t cmd = s->fill_command;
    if (!cmd)
    {
        return;
    }

    switch (cmd)
    {
    case CMD_FILL_N2:
        smf_set_state(SMF_CTX(s), &states[FILL]);
        break;
    case CMD_FILL_PRE_PRESS:
        smf_set_state(SMF_CTX(s), &states[PRE_PRESS]);
        break;
    case CMD_FILL_N2O:
        smf_set_state(SMF_CTX(s), &states[FILL_N2O]);
        break;
    case CMD_FILL_POST_PRESS:
        smf_set_state(SMF_CTX(s), &states[POST_PRESS]);
        break;
    default:
        smf_set_handled(SMF_CTX(s));
    }

    cmd = 0;
}

static void fill_exit(void *o)
{
    ARG_UNUSED(o);
}

static void ready_entry(void *o)
{
    ARG_UNUSED(o);
}

static void ready_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;

    command_t cmd = s->command;
    if (!cmd)
    {
        return;
    }

    switch (cmd)
    {
    case CMD_ARM:
        smf_set_state(SMF_CTX(s), &states[ARMED]);
        break;
    case CMD_ABORT:
        smf_set_state(SMF_CTX(s), &states[ABORT]);
        break;
    default:
        smf_set_handled(SMF_CTX(s));
    }

    cmd = 0;
}

static void ready_exit(void *o)
{
    ARG_UNUSED(o);
}

static void armed_entry(void *o)
{
    ARG_UNUSED(o);
}

static void armed_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;

    command_t cmd = s->command;
    if (!cmd)
    {
        return;
    }

    switch (cmd)
    {
    case CMD_FIRE:
        if (s->data.thermocouples.chamber_thermo >
            s->config->flight_sm_config.min_chamber_launch_temp)
        {
            // Chamber temperature is above minimum
        }
        else
        {
            smf_set_handled(SMF_CTX(s));
            break;
        }
        smf_set_state(SMF_CTX(s), &states[FLIGHT]);
        break;
    default:
        smf_set_handled(SMF_CTX(s));
    }

    cmd = 0;
}

static void armed_exit(void *o)
{
    ARG_UNUSED(o);
}

static void flight_entry(void *o)
{
    ARG_UNUSED(o);
}

static void flight_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    command_t cmd = s->command;
    if (!cmd)
    {
        return;
    }
    switch (cmd)
    {
    case CMD_LAUNCH_OVERRIDE:
        // Handle launch override if necessary
        break;
    case CMD_ABORT:
        smf_set_state(SMF_CTX(s), &states[ABORT]);
        break;
    default:
        smf_set_handled(SMF_CTX(s));
    }
}

static void flight_exit(void *o)
{
    ARG_UNUSED(o);
}

static void abort_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    (void)s;
    close_all_valves(s);
    set_valve(s, VALVE_ABORT, true);
    set_valve(s, VALVE_PRESSURIZING, true);
}

static void abort_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;

    command_t cmd = s->command;
    if (!cmd)
    {
        return;
    }

    switch (cmd)
    {
    case CMD_READY:
        smf_set_state(SMF_CTX(s), &states[IDLE]);
        break;
    case CMD_STOP:
        smf_set_state(SMF_CTX(s), &states[IDLE]);
        break;
    default:
        smf_set_handled(SMF_CTX(s));
    }

    cmd = 0;
}

static void abort_exit(void *o)
{
    ARG_UNUSED(o);
}

const struct smf_state states[] = {
    // clang-format off
    [ROOT]   = SMF_CREATE_STATE(root_entry, root_run, root_exit, NULL, &states[IDLE]),
    [IDLE]   = SMF_CREATE_STATE(idle_entry, idle_run, idle_exit, &states[ROOT], NULL),
    [ABORT]  = SMF_CREATE_STATE(abort_entry, abort_run, abort_exit,&states[ROOT], NULL),
    [FILL]   = SMF_CREATE_STATE(fill_entry, fill_run, fill_exit, &states[ROOT], NULL),
    [READY]  = SMF_CREATE_STATE(ready_entry, ready_run, ready_exit, &states[ROOT], NULL),
    [ARMED]  = SMF_CREATE_STATE(armed_entry, armed_run, armed_exit, &states[ROOT], NULL),
    [FLIGHT] = SMF_CREATE_STATE(flight_entry, flight_run, flight_exit, &states[ROOT], NULL),

    [FILL_SST(SAFE_PAUSE)]      = SMF_CREATE_STATE(safe_pause_entry, safe_pause_run, safe_pause_exit, &states[FILL], NULL),
    [FILL_SST(SAFE_PAUSE_IDLE)] = SMF_CREATE_STATE(safe_pause_idle_entry, safe_pause_idle_run, safe_pause_idle_exit, &states[FILL_SST(SAFE_PAUSE)], NULL),
    [FILL_SST(SAFE_PAUSE_VENT)] = SMF_CREATE_STATE(safe_pause_vent_entry, safe_pause_vent_run, safe_pause_vent_exit, &states[FILL_SST(SAFE_PAUSE)], NULL),

    [FILL_SST(FILL_N2)]      = SMF_CREATE_STATE(fill_n2_entry, fill_n2_run, fill_n2_exit, &states[FILL], NULL),
    [FILL_SST(FILL_N2_IDLE)] = SMF_CREATE_STATE(fill_n2_idle_entry, fill_n2_idle_run, fill_n2_idle_exit, &states[FILL_SST(FILL)], NULL),
    [FILL_SST(FILL_N2_FILL)] = SMF_CREATE_STATE(fill_n2_fill_entry, fill_n2_fill_run, fill_n2_fill_exit, &states[FILL_SST(FILL)], NULL),
    [FILL_SST(FILL_N2_VENT)] = SMF_CREATE_STATE(fill_n2_vent_entry, fill_n2_vent_run, fill_n2_vent_exit, &states[FILL_SST(FILL)], NULL),

    [FILL_SST(PRE_PRESS)]         = SMF_CREATE_STATE(pre_press_entry, pre_press_run, pre_press_exit, &states[FILL], NULL),
    [FILL_SST(PRE_PRESS_IDLE)]    = SMF_CREATE_STATE(pre_press_idle_entry, pre_press_idle_run, pre_press_idle_exit, &states[FILL_SST(PRE_PRESS)], NULL),
    [FILL_SST(PRE_PRESS_FILL_N2)] = SMF_CREATE_STATE(pre_press_fill_entry, pre_press_fill_run, pre_press_fill_exit, &states[FILL_SST(PRE_PRESS)], NULL),
    [FILL_SST(PRE_PRESS_VENT)]    = SMF_CREATE_STATE(pre_press_vent_entry, pre_press_vent_run, pre_press_vent_exit, &states[FILL_SST(PRE_PRESS)], NULL),

    [FILL_SST(FILL_N2O)]      = SMF_CREATE_STATE(fill_n2o_entry, fill_n2o_run, fill_n2o_exit, &states[FILL], NULL),
    [FILL_SST(FILL_N2O_IDLE)] = SMF_CREATE_STATE(fill_n2o_idle_entry, fill_n2o_idle_run, fill_n2o_idle_exit, &states[FILL_SST(FILL_N2O)], NULL),
    [FILL_SST(FILL_N2O_FILL)] = SMF_CREATE_STATE(fill_n2o_fill_entry, fill_n2o_fill_run, fill_n2o_fill_exit, &states[FILL_SST(FILL_N2O)], NULL),
    [FILL_SST(FILL_N2O_VENT)] = SMF_CREATE_STATE(fill_n2o_vent_entry, fill_n2o_vent_run, fill_n2o_vent_exit, &states[FILL_SST(FILL_N2O)], NULL),

    [FILL_SST(POST_PRESS)]         = SMF_CREATE_STATE(post_press_entry, post_press_run, post_press_exit, &states[FILL], NULL),
    [FILL_SST(POST_PRESS_IDLE)]    = SMF_CREATE_STATE(post_press_idle_entry, post_press_idle_run, post_press_idle_exit, &states[FILL_SST(POST_PRESS)], NULL),
    [FILL_SST(POST_PRESS_FILL_N2)] = SMF_CREATE_STATE(post_press_fill_entry, post_press_fill_run, post_press_fill_exit, &states[FILL_SST(POST_PRESS)], NULL),
    [FILL_SST(POST_PRESS_VENT)]    = SMF_CREATE_STATE(post_press_vent_entry, post_press_vent_run, post_press_vent_exit, &states[FILL_SST(POST_PRESS)], NULL),

    // clang-format on
};

void sm_init(struct sm_object *initial_s_obj)
{
    smf_set_initial(SMF_CTX(initial_s_obj), &states[IDLE]);
}
