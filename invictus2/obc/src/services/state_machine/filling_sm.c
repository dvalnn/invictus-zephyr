#include "services/state_machine/filling_sm.h"

/* ======================================================================== */
/* fill/safe_pause: All valves closed, waiting for resume command. */
/* ======================================================================== */
static void safe_pause_entry(void *o) {
    // Close all valves for safety.
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}

static void safe_pause_run(void *o) {
    // Wait for CMD_RESUME to transition to idle.
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

static void safe_pause_exit(void *o) {
    // Exiting safe pause.
    struct sm_object *s = (struct sm_object *)o;
}

/* ======================================================================== */
/* fill/safe_pause/idle: Idle during safe pause, all valves closed. */
/* ======================================================================== */
static void safe_pause_idle_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}

static void safe_pause_idle_run(void *o) {
    // Transition to vent if N2O tank pressure exceeds trigger.
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.pressures.n2o_tank_pressure > s->config->filling_sm_config.safe_pause.trigger_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[SM_FILL_SAFE_PAUSE_VENT]);
    }
}

static void safe_pause_idle_exit(void *o) {
    struct sm_object *s = (struct sm_object *)o;
}

/* ======================================================================== */
/* fill/safe_pause/vent: Venting during safe pause. */
/* ======================================================================== */
static void safe_pause_vent_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    open_single_valve(s, VALVE_VENT);
}

static void safe_pause_vent_run(void *o) {
    // Transition back to idle when pressure is safe.
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.n2o_tank_pressure <= s->config->safe_pause.target_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[SM_FILL_SAFE_PAUSE_IDLE]);
    }
}

static void safe_pause_vent_exit(void *o) {
    struct sm_object *s = (struct sm_object *)o;
}

/* ======================================================================== */
/* fill/filling_n2: COPV filling sequence. */
/* ======================================================================== */
static void filling_copv_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}

static void filling_copv_run(void *o) {
    // No operation in entry state.
    ARG_UNUSED(o);
}

static void filling_copv_exit(void *o) {
    struct sm_object *s = (struct sm_object *)o;
}

/* ======================================================================== */
/* fill/filling_n2/idle: Idle during COPV filling. */
/* ======================================================================== */
static void filling_copv_idle_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}

static void filling_copv_idle_run(void *o) {
    // Transition to fill when pressure is below target.
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.n2_tank_pressure <= s->config->f_copv.target_n2_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[SM_FILL_FILL_N2_FILL]);
    }
}

static void filling_copv_idle_exit(void *o) {
    struct sm_object *s = (struct sm_object *)o;
}

/* ======================================================================== */
/* fill/filling_n2/fill: Filling COPV with N2. */
/* ======================================================================== */
static void filling_copv_fill_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    open_single_valve(s, VALVE_N2_FILL);
}

static void filling_copv_fill_run(void *o) {
    // Transition to idle when pressure reaches target.
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.n2_tank_pressure >= s->config->f_copv.target_n2_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[SM_FILL_FILL_N2_IDLE]);
    }
}

static void filling_copv_fill_exit(void *o) {
    struct sm_object *s = (struct sm_object *)o;
}

/* ======================================================================== */
/* fill/pre_press: Pre-pressurization sequence. */
/* ======================================================================== */
static void pre_press_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}

static void pre_press_run(void *o) {
    ARG_UNUSED(o);
}

static void pre_press_exit(void *o) {
    struct sm_object *s = (struct sm_object *)o;
}

/* ======================================================================== */
/* fill/pre_press/idle: Idle during pre-pressurization. */
/* ======================================================================== */
static void pre_press_idle_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}

static void pre_press_idle_run(void *o) {
    // Transition to vent or fill based on pressure.
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.n2o_tank_pressure > s->config->pre_p.trigger_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[SM_FILL_PRE_PRESS_VENT]);
        return;
    }
    if (s->data.n2o_tank_pressure < s->config->pre_p.target_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[SM_FILL_PRE_PRESS_FILL_N2]);
    }
}

static void pre_press_idle_exit(void *o) {
    struct sm_object *s = (struct sm_object *)o;
}

/* ======================================================================== */
/* fill/pre_press/vent: Venting during pre-pressurization. */
/* ======================================================================== */
static void pre_press_vent_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    open_single_valve(s, VALVE_VENT);
}

static void pre_press_vent_run(void *o) {
    // Transition to idle when pressure is safe.
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.n2o_tank_pressure <= s->config->pre_p.target_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[SM_FILL_PRE_PRESS_IDLE]);
    }
}

static void pre_press_vent_exit(void *o) {
    struct sm_object *s = (struct sm_object *)o;
}

/* ======================================================================== */
/* fill/pre_press/fill_n2: Filling N2 during pre-pressurization. */
/* ======================================================================== */
static void pre_press_fill_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    open_single_valve(s, VALVE_PRESSURIZING);
}

static void pre_press_fill_run(void *o) {
    // Transition to idle when pressure reaches target.
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.n2o_tank_pressure >= s->config->pre_p.target_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[SM_FILL_PRE_PRESS_IDLE]);
    }
}

static void pre_press_fill_exit(void *o) {
    struct sm_object *s = (struct sm_object *)o;
}

/* ======================================================================== */
/* fill/filling_n2o: N2O filling sequence. */
/* ======================================================================== */
static void filling_n2o_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}

static void filling_n2o_run(void *o) {
    ARG_UNUSED(o);
}

static void filling_n2o_exit(void *o) {
    struct sm_object *s = (struct sm_object *)o;
}

/* ======================================================================== */
/* fill/filling_n2o/idle: Idle during N2O filling. */
/* ======================================================================== */
static void filling_n2o_idle_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}

static void filling_n2o_idle_run(void *o) {
    // Transition to fill when tank weight is below target.
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.n2o_tank_weight < s->config->f_n2o.target_n2o_tank_weight) {
        smf_set_state(SMF_CTX(s), &states[SM_FILL_FILL_N2O_FILL]);
    }
}

static void filling_n2o_idle_exit(void *o) {
    struct sm_object *s = (struct sm_object *)o;
}

/* ======================================================================== */
/* fill/filling_n2o/fill: Filling N2O tank. */
/* ======================================================================== */
static void filling_n2o_fill_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    open_single_valve(s, VALVE_N2O_FILL);
}

static void filling_n2o_fill_run(void *o) {
    // Transition to vent if pressure and temperature exceed triggeSM,
    // or to idle if weight reaches target.
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.pressures.n2o_tank_pressure >= s->config->filling_sm_config.f_n2o.trigger_n2o_tank_pressure &&
        s->data.pressures.n2o_tank_temperature > s->config->filling_sm_config.f_n2o.trigger_n2o_tank_temperature) {
        smf_set_state(SMF_CTX(s), &states[SM_FILL_FILL_N2O_VENT]);
        return;
    }
    if (s->data.loadcells.n2o_loadcell >= s->config->filling_sm_config.f_n2o.target_n2o_tank_weight) {
        smf_set_state(SMF_CTX(s), &states[SM_FILL_FILL_N2O_IDLE]);
    }
}

static void filling_n2o_fill_exit(void *o) {
    struct sm_object *s = (struct sm_object *)o;
}

/* ======================================================================== */
/* fill/filling_n2o/vent: Venting during N2O filling. */
/* ======================================================================== */
static void filling_n2o_vent_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
    toggle_valve(s, VALVE_N2O_FILL);
    toggle_valve(s, VALVE_VENT);
}

static void filling_n2o_vent_run(void *o) {
    // Transition to fill when pressure or temperature drop below triggeSM.
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.pressures.n2o_tank_pressure <= s->config->filling_sm_config.f_n2o.target_n2o_tank_pressure ||
        s->data.pressures.n2o_tank_temperature <= s->config->filling_sm_config.f_n2o.trigger_n2o_tank_temperature) {
        smf_set_state(SMF_CTX(s), &states[SM_FILL_FILL_N2O_FILL]);
    }
}

static void filling_n2o_vent_exit(void *o) {
    struct sm_object *s = (struct sm_object *)o;
}

/* ======================================================================== */
/* fill/post_press: Post-pressurization sequence. */
/* ======================================================================== */
static void post_press_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}

static void post_press_run(void *o) {
    ARG_UNUSED(o);
}

static void post_press_exit(void *o) {
    struct sm_object *s = (struct sm_object *)o;
}

/* ======================================================================== */
/* fill/post_press/idle: Idle during post-pressurization. */
/* ======================================================================== */
static void post_press_idle_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}

static void post_press_idle_run(void *o) {
    // Transition to vent or fill based on pressure.
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.pressures.n2o_tank_pressure > s->config->filling_sm_config.post_p.trigger_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[SM_FILL_POST_PRESS_VENT]);
        return;
    }
    if (s->data.pressures.n2o_tank_pressure < s->config->filling_sm_config.post_p.target_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[SM_FILL_POST_PRESS_FILL_N2]);
    }
}

static void post_press_idle_exit(void *o) {
    struct sm_object *s = (struct sm_object *)o;
}

/* ======================================================================== */
/* fill/post_press/fill_n2: Filling N2 during post-pressurization. */
/* ======================================================================== */
static void post_press_fill_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    open_single_valve(s, VALVE_N2_FILL);
}

static void post_press_fill_run(void *o) {
    // Transition to idle when pressure reaches target.
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.pressures.n2o_tank_pressure >= s->config->filling_sm_config.post_p.target_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[SM_FILL_POST_PRESS_IDLE]);
    }
}

static void post_press_fill_exit(void *o) {
    struct sm_object *s = (struct sm_object *)o;
}

/* ======================================================================== */
/* fill/post_press/vent: Venting during post-pressurization. */
/* ======================================================================== */
static void post_press_vent_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    open_single_valve(s, VALVE_VENT);
}

static void post_press_vent_run(void *o) {
    // Transition to idle when pressure is safe.
    struct sm_object *s = (struct sm_object *)o;
    if (CMD_GLOBAL(s->command)) return;
    if (s->data.pressures.n2o_tank_pressure <= s->config->filling_sm_config.post_p.target_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[SM_FILL_POST_PRESS_IDLE]);
    }
}

static void post_press_vent_exit(void *o) {
    struct sm_object *s = (struct sm_object *)o;
}
