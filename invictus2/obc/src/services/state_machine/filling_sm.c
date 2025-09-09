#include "services/state_machine/filling_sm.h"

LOG_MODULE_REGISTER(filling_sm, LOG_LEVEL_DBG);

// FILL_SAFE_PAUSE
static void safe_pause_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}
static void safe_pause_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    command_t cmd = s->command;
    if (!cmd) return;
    switch (cmd) {
        case CMD_RESUME:
            smf_set_state(SMF_CTX(s), &states[SM_IDLE]);
            break;
        default:
            smf_set_handled(SMF_CTX(s));
    }
}

static void safe_pause_exit(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
}

// FILL_SAFE_PAUSE_IDLE
static void safe_pause_idle_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}
static void safe_pause_idle_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.pressures.n2o_tank_pressure > s->config->filling_sm_config.safe_pause.trigger_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[SM_FILL_SAFE_PAUSE_VENT]);
    }
}
static void safe_pause_idle_exit(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
}

// FILL_SAFE_PAUSE_VENT
static void safe_pause_vent_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->valve_states.raw = 0;
    s->valve_states.vent = 1;
}
static void safe_pause_vent_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.n2o_tank_pressure <= s->config->safe_pause.target_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[RS_FILL_SAFE_PAUSE_IDLE]);
    }
}
static void safe_pause_vent_exit(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
}

// FILL_FILLING_N2
static void filling_copv_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->valve_states.raw = 0;
}
static void filling_copv_run(void *o)
{
    ARG_UNUSED(o);
}
static void filling_copv_exit(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
}

// FILL_FILLING_N2_IDLE
static void filling_copv_idle_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->valve_states.raw = 0;
}
static void filling_copv_idle_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.n2_tank_pressure <= s->config->f_copv.target_n2_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[RS_FILL_FILLING_N2_FILL]);
    }
}
static void filling_copv_idle_exit(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
}

// FILL_FILLING_N2_FILL
static void filling_copv_fill_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->valve_states.raw = 0;
    s->valve_states.n2_fill = 1;
}
static void filling_copv_fill_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.n2_tank_pressure >= s->config->f_copv.target_n2_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[RS_FILL_FILLING_N2_IDLE]);
    }
}
static void filling_copv_fill_exit(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
}

// FILL_PRE_PRESS
static void pre_press_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->valve_states.raw = 0;
}
static void pre_press_run(void *o)
{
    ARG_UNUSED(o);
}
static void pre_press_exit(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
}

// FILL_PRE_PRESS_IDLE
static void pre_press_idle_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->valve_states.raw = 0;
}
static void pre_press_idle_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.n2o_tank_pressure > s->config->pre_p.trigger_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[RS_FILL_PRE_PRESS_VENT]);
        return;
    }
    if (s->data.n2o_tank_pressure < s->config->pre_p.target_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[RS_FILL_PRE_PRESS_FILL_N2]);
    }
}
static void pre_press_idle_exit(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
}

// FILL_PRE_PRESS_VENT
static void pre_press_vent_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->valve_states.raw = 0;
    s->valve_states.vent = 1;
}
static void pre_press_vent_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.n2o_tank_pressure <= s->config->pre_p.target_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[RS_FILL_PRE_PRESS_IDLE]);
    }
}
static void pre_press_vent_exit(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
}

// FILL_PRE_PRESS_FILL_N2
static void pre_press_fill_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->valve_states.raw = 0;
    s->valve_states.pressurizing = 1;
}
static void pre_press_fill_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.n2o_tank_pressure >= s->config->pre_p.target_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[RS_FILL_PRE_PRESS_IDLE]);
    }
}
static void pre_press_fill_exit(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
}

// FILL_FILLING_N2O
static void filling_n2o_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->valve_states.raw = 0;
}
static void filling_n2o_run(void *o)
{
    ARG_UNUSED(o);
}
static void filling_n2o_exit(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
}

// FILL_FILLING_N2O_IDLE
static void filling_n2o_idle_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->valve_states.raw = 0;
}
static void filling_n2o_idle_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.n2o_tank_weight < s->config->f_n2o.target_n2o_tank_weight) {
        smf_set_state(SMF_CTX(s), &states[RS_FILL_FILLING_N2O_FILL]);
    }
}
static void filling_n2o_idle_exit(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
}

// FILL_FILLING_N2O_FILL
static void filling_n2o_fill_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->valve_states.raw = 0;
    s->valve_states.n2o_fill = 1;
}
static void filling_n2o_fill_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.n2o_tank_pressure >= s->config->f_n2o.trigger_n2o_tank_pressure &&
        s->data.n2o_tank_temperature > s->config->f_n2o.trigger_n2o_tank_temperature) {
        smf_set_state(SMF_CTX(s), &states[RS_FILL_FILLING_N2O_VENT]);
        return;
    }
    if (s->data.n2o_tank_weight >= s->config->f_n2o.target_n2o_tank_weight) {
        smf_set_state(SMF_CTX(s), &states[RS_FILL_FILLING_N2O_IDLE]);
    }
}
static void filling_n2o_fill_exit(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
}

// FILL_FILLING_N2O_VENT
static void filling_n2o_vent_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->valve_states.raw = 0;
    s->valve_states.n2o_fill = 1;
    s->valve_states.vent = 1;
}
static void filling_n2o_vent_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.n2o_tank_pressure <= s->config->f_n2o.target_n2o_tank_pressure ||
        s->data.n2o_tank_temperature <= s->config->f_n2o.trigger_n2o_tank_temperature) {
        smf_set_state(SMF_CTX(s), &states[RS_FILL_FILLING_N2O_FILL]);
    }
}
static void filling_n2o_vent_exit(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
}

// FILL_POST_PRESS
static void post_press_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->valve_states.raw = 0;
}
static void post_press_run(void *o)
{
    ARG_UNUSED(o);
}
static void post_press_exit(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
}

// FILL_POST_PRESS_IDLE
static void post_press_idle_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->valve_states.raw = 0;
}
static void post_press_idle_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.n2o_tank_pressure > s->config->post_p.trigger_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[RS_FILL_POST_PRESS_VENT]);
        return;
    }
    if (s->data.n2o_tank_pressure < s->config->post_p.target_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[RS_FILL_POST_PRESS_FILL_N2]);
    }
}
static void post_press_idle_exit(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
}

// FILL_POST_PRESS_FILL_N2
static void post_press_fill_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->valve_states.raw = 0;
    s->valve_states.n2_fill = 1;
}
static void post_press_fill_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.n2o_tank_pressure >= s->config->post_p.target_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[RS_FILL_POST_PRESS_IDLE]);
    }
}
static void post_press_fill_exit(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
}

// FILL_POST_PRESS_VENT
static void post_press_vent_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->valve_states.raw = 0;
    s->valve_states.vent = 1;
}
static void post_press_vent_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.n2o_tank_pressure <= s->config->post_p.target_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[RS_FILL_POST_PRESS_IDLE]);
    }
}
static void post_press_vent_exit(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
}
