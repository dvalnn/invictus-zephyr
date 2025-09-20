#include "services/state_machine/filling_sm.h"

/* ======================================================================== */
/* fill/safe_pause: All valves closed, waiting for resume command. */
/* ======================================================================== */
void safe_pause_entry(void *o) {
    // Close all valves for safety.
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}

void safe_pause_run(void *o) {
    // Wait for CMD_RESUME to transition to idle.
    struct sm_object *s = (struct sm_object *)o;
    command_t cmd = s->command;
    if (!cmd) return;
    switch (cmd) {
        case CMD_RESUME:
            smf_set_state(SMF_CTX(s), &states[IDLE]);
            break;
        default:
            smf_set_handled(SMF_CTX(s));
    }
}

void safe_pause_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* fill/safe_pause/idle: Idle during safe pause, all valves closed. */
/* ======================================================================== */
void safe_pause_idle_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}

void safe_pause_idle_run(void *o) {
    // Transition to vent if N2O tank pressure exceeds trigger.
    struct sm_object *s = (struct sm_object *)o;

    if (s->data.pressures.n2o_tank_pressure > s->config->filling_sm_config.safe_pause.trigger_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[SAFE_PAUSE_VENT]);
    }
}

void safe_pause_idle_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* fill/safe_pause/vent: Venting during safe pause. */
/* ======================================================================== */
void safe_pause_vent_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    open_single_valve(s, VALVE_VENT);
}

void safe_pause_vent_run(void *o) {
    // Transition back to idle when pressure is safe.
    struct sm_object *s = (struct sm_object *)o;

    if (s->data.pressures.n2o_tank_pressure <= s->config->filling_sm_config.safe_pause.target_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[SAFE_PAUSE_IDLE]);
    }
}

void safe_pause_vent_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* fill/filling_n2: COPV filling sequence. */
/* ======================================================================== */
void fill_n2_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}

void fill_n2_run(void *o) {
    ARG_UNUSED(o);
}

void fill_n2_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* fill/fill_n2/idle: Idle during COPV filling. */
/* ======================================================================== */
void fill_n2_idle_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}

void fill_n2_idle_run(void *o) {
    // Transition to fill when pressure is below target.
    struct sm_object *s = (struct sm_object *)o;

    if (s->data.pressures.n2_line_pressure <= s->config->filling_sm_config.f_copv.target_n2_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[FILL_N2_FILL]);
    } else if (s->data.pressures.n2_line_pressure > s->config->filling_sm_config.f_copv.trigger_n2_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[FILL_N2_VENT]);
    }
}

void fill_n2_idle_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* fill/fill_n2/fill: Filling COPV with N2. */
/* ======================================================================== */
void fill_n2_fill_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    open_single_valve(s, VALVE_N2_FILL);
}

void fill_n2_fill_run(void *o) {
    // Transition to idle when pressure reaches target.
    struct sm_object *s = (struct sm_object *)o;

    if (s->data.pressures.n2_line_pressure >= s->config->filling_sm_config.f_copv.target_n2_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[FILL_N2_IDLE]);
    }

}

void fill_n2_fill_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* fill/fill_n2/vent: Venting during N2 filling. */
/* ======================================================================== */

void fill_n2_vent_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
    toggle_valve(s, VALVE_N2_FILL, true);
    toggle_valve(s, VALVE_VENT, true);
}

void fill_n2_vent_run(void *o) {
    // Transition to idle when pressure reaches target.
    struct sm_object *s = (struct sm_object *)o;
    if (s->data.pressures.n2_line_pressure <= s->config->filling_sm_config.f_copv.target_n2_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[FILL_N2_IDLE]);
    }
}

void fill_n2_vent_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* fill/pre_press: Pre-pressurization sequence. */
/* ======================================================================== */
void pre_press_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}

void pre_press_run(void *o) {
    ARG_UNUSED(o);
}

void pre_press_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* fill/pre_press/idle: Idle during pre-pressurization. */
/* ======================================================================== */
void pre_press_idle_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}

void pre_press_idle_run(void *o) {
    // Transition to vent or fill based on pressure.
    struct sm_object *s = (struct sm_object *)o;
    if (s->data.pressures.n2o_tank_pressure > s->config->filling_sm_config.pre_p.trigger_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[PRE_PRESS_VENT]);
        return;
    }
    if (s->data.pressures.n2o_tank_pressure < s->config->filling_sm_config.pre_p.target_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[PRE_PRESS_FILL_N2]);
    }
}

void pre_press_idle_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* fill/pre_press/vent: Venting during pre-pressurization. */
/* ======================================================================== */
void pre_press_vent_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    open_single_valve(s, VALVE_VENT);
}

void pre_press_vent_run(void *o) {
    // Transition to idle when pressure is safe.
    struct sm_object *s = (struct sm_object *)o;
    if (s->data.pressures.n2o_tank_pressure <= s->config->filling_sm_config.pre_p.target_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[PRE_PRESS_IDLE]);
    }
}

void pre_press_vent_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* fill/pre_press/fill_n2: Filling N2 during pre-pressurization. */
/* ======================================================================== */
void pre_press_fill_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    open_single_valve(s, VALVE_PRESSURIZING);
}

void pre_press_fill_run(void *o) {
    // Transition to idle when pressure reaches target.
    struct sm_object *s = (struct sm_object *)o;

    if (s->data.pressures.n2o_tank_pressure >= s->config->filling_sm_config.pre_p.target_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[PRE_PRESS_IDLE]);
    }
}

void pre_press_fill_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* fill/fill_n2o: N2O filling sequence. */
/* ======================================================================== */
void fill_n2o_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}

void fill_n2o_run(void *o) {
    ARG_UNUSED(o);
}

void fill_n2o_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* fill/fill_n2o/idle: Idle during N2O filling. */
/* ======================================================================== */
void fill_n2o_idle_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}

void fill_n2o_idle_run(void *o) {
    // Transition to fill when tank weight is below target.
    struct sm_object *s = (struct sm_object *)o;

    if (s->data.loadcells.n2o_loadcell < s->config->filling_sm_config.f_n2o.target_n2o_tank_weight) {
        smf_set_state(SMF_CTX(s), &states[FILL_N2O_FILL]);
    }
}

void fill_n2o_idle_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* fill/fill_n2o/fill: Filling N2O tank. */
/* ======================================================================== */
void fill_n2o_fill_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    open_single_valve(s, VALVE_N2O_FILL);
}

void fill_n2o_fill_run(void *o) {
    // FIXME: should be minimum temp thermocouple not specific one
    // Transition to vent if pressure and temperature exceed trigger,
    // or to idle if weight reaches target.
    struct sm_object *s = (struct sm_object *)o;
    
    if (s->data.pressures.n2o_tank_pressure >= s->config->filling_sm_config.f_n2o.trigger_n2o_tank_pressure &&
        s->data.thermocouples.n2o_tank_uf_t1 > s->config->filling_sm_config.f_n2o.trigger_n2o_tank_temperature) {
        smf_set_state(SMF_CTX(s), &states[FILL_N2O_VENT]);
        return;
    }
    if (s->data.loadcells.n2o_loadcell >= s->config->filling_sm_config.f_n2o.target_n2o_tank_weight) {
        smf_set_state(SMF_CTX(s), &states[FILL_N2O_IDLE]);
    }
}

void fill_n2o_fill_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* fill/fill_n2o/vent: Venting during N2O filling. */
/* ======================================================================== */
void fill_n2o_vent_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
    toggle_valve(s, VALVE_N2O_FILL, true);
    toggle_valve(s, VALVE_VENT, true);
}

void fill_n2o_vent_run(void *o) {
    // Transition to fill when pressure or temperature drop below trigger.
    // FIXME: Instead of specific thermocouple should be lowest temp one.
    struct sm_object *s = (struct sm_object *)o;
    
    if (s->data.pressures.n2o_tank_pressure <= s->config->filling_sm_config.f_n2o.target_n2o_tank_pressure ||
        s->data.thermocouples.n2o_tank_uf_t1 <= s->config->filling_sm_config.f_n2o.trigger_n2o_tank_temperature) {
        smf_set_state(SMF_CTX(s), &states[FILL_N2O_FILL]);
    }
}

void fill_n2o_vent_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* fill/post_press: Post-pressurization sequence. */
/* ======================================================================== */
void post_press_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}

void post_press_run(void *o) {
    ARG_UNUSED(o);
}

void post_press_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* fill/post_press/idle: Idle during post-pressurization. */
/* ======================================================================== */
void post_press_idle_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}

void post_press_idle_run(void *o) {
    // Transition to vent or fill based on pressure.
    struct sm_object *s = (struct sm_object *)o;
    
    if (s->data.pressures.n2o_tank_pressure > s->config->filling_sm_config.post_p.trigger_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[POST_PRESS_VENT]);
        return;
    }
    if (s->data.pressures.n2o_tank_pressure < s->config->filling_sm_config.post_p.target_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[POST_PRESS_FILL_N2]);
    }
}

void post_press_idle_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* fill/post_press/fill_n2: Filling N2 during post-pressurization. */
/* ======================================================================== */
void post_press_fill_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    open_single_valve(s, VALVE_N2_FILL);
}

void post_press_fill_run(void *o) {
    // Transition to idle when pressure reaches target.
    struct sm_object *s = (struct sm_object *)o;
    
    if (s->data.pressures.n2o_tank_pressure >= s->config->filling_sm_config.post_p.target_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[POST_PRESS_IDLE]);
    }
}

void post_press_fill_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* fill/post_press/vent: Venting during post-pressurization. */
/* ======================================================================== */
void post_press_vent_entry(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    open_single_valve(s, VALVE_VENT);
}

void post_press_vent_run(void *o) {
    // Transition to idle when pressure is safe.
    struct sm_object *s = (struct sm_object *)o;
    
    if (s->data.pressures.n2o_tank_pressure <= s->config->filling_sm_config.post_p.target_n2o_tank_pressure) {
        smf_set_state(SMF_CTX(s), &states[POST_PRESS_IDLE]);
    }
}

void post_press_vent_exit(void *o) {
    ARG_UNUSED(o);
}
