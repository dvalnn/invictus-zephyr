#include "services/state_machine/filling_sm.h"
#include "services/state_machine/main_sm.h"

#include <zephyr/toolchain.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_DECLARE(state_machine_service);

#define SET_FILL_STATE(s, st) SET_STATE(s, FILL_SST(st))

/* ======================================================================== */
/* root/fill/safe_pause: Filling is paused, waiting for resume command.     */
/* ======================================================================== */
void safe_pause_entry(void *o)
{
    // Close all valves for safety.
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.filling_state = SAFE_PAUSE;
    close_all_valves(s);
}

void safe_pause_run(void *o)
{
    // Wait for CMD_RESUME to transition to idle.
    struct sm_object *s = (struct sm_object *)o;
    command_t cmd = s->command;
    if (!cmd)
    {
        return;
    }

    switch (cmd)
    {
    case CMD_RESUME:
        SET_STATE(s, IDLE);
        break;
    default:
        LOG_WRN("Unexpected command in SAFE_PAUSE: 0x%08x", cmd);
        SET_HANDLED(s);
    }
}

void safe_pause_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* root/fill/safe_pause/idle: Idle. All valves closed.                      */
/* ======================================================================== */
void safe_pause_idle_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.filling_state = SAFE_PAUSE_IDLE;
    close_all_valves(s);
}

void safe_pause_idle_run(void *o)
{
    // Transition to vent if N2O tank pressure exceeds trigger.
    struct sm_object *s = (struct sm_object *)o;

    if (s->data.pressures.n2o_tank_pressure >
        s->config->filling_sm_config.safe_pause.trigger_n2o_tank_pressure)
    {
        SET_FILL_STATE(s, SAFE_PAUSE_VENT);
    }
}

void safe_pause_idle_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* root/fill/safe_pause/vent: Vent valve open                               */
/* ======================================================================== */
void safe_pause_vent_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.filling_state = SAFE_PAUSE_VENT;
    open_single_valve(s, VALVE_VENT);
}

void safe_pause_vent_run(void *o)
{
    // Transition back to idle when pressure is safe.
    struct sm_object *s = (struct sm_object *)o;

    if (s->data.pressures.n2o_tank_pressure <=
        s->config->filling_sm_config.safe_pause.target_n2o_tank_pressure)
    {
        SET_FILL_STATE(s, SAFE_PAUSE_IDLE);
    }
}
void safe_pause_vent_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* root/fill/filling_n2: COPV filling sequence.                             */
/* ======================================================================== */
void fill_n2_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.filling_state = FILL_N2;
    close_all_valves(s);
}

void fill_n2_run(void *o)
{
    ARG_UNUSED(o);
}

void fill_n2_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* root/fill/fill_n2/idle: Idle during COPV filling.                        */
/* ======================================================================== */
void fill_n2_idle_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.filling_state = FILL_N2_IDLE;
    close_all_valves(s);
}

void fill_n2_idle_run(void *o)
{
    // Transition to fill when pressure is below target.
    struct sm_object *s = (struct sm_object *)o;

    if (s->data.pressures.n2_line_pressure <=
        s->config->filling_sm_config.fill_n2.target_n2_tank_pressure)
    {
        SET_FILL_STATE(s, FILL_N2_FILL);
    }
    else if (s->data.pressures.n2_line_pressure >
             s->config->filling_sm_config.fill_n2.trigger_n2_tank_pressure)
    {
        SET_FILL_STATE(s, FILL_N2_VENT);
    }
}

void fill_n2_idle_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* root/fill/fill_n2/fill: Filling COPV with N2.                            */
/* ======================================================================== */
void fill_n2_fill_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.filling_state = FILL_N2_FILL;
    open_single_valve(s, VALVE_N2_FILL);
}

void fill_n2_fill_run(void *o)
{
    // Transition to idle when pressure reaches target.
    struct sm_object *s = (struct sm_object *)o;

    if (s->data.pressures.n2_line_pressure >=
        s->config->filling_sm_config.fill_n2.target_n2_tank_pressure)
    {
        SET_FILL_STATE(s, FILL_N2_IDLE);
    }
}

void fill_n2_fill_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* root/fill/fill_n2/vent: Venting during N2 filling.                       */
/* ======================================================================== */

void fill_n2_vent_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.filling_state = FILL_N2_VENT;
    close_all_valves(s);
    set_valve(s, VALVE_N2_FILL, true);
    set_valve(s, VALVE_VENT, true);
}

void fill_n2_vent_run(void *o)
{
    // Transition to idle when pressure reaches target.
    struct sm_object *s = (struct sm_object *)o;
    if (s->data.pressures.n2_line_pressure <=
        s->config->filling_sm_config.fill_n2.target_n2_tank_pressure)
    {
        SET_FILL_STATE(s, FILL_N2_IDLE);
    }
}

void fill_n2_vent_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* root/fill/pre_press: Pre-pressurization sequence.                        */
/* ======================================================================== */
void pre_press_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.filling_state = PRE_PRESS;
    close_all_valves(s);
}

void pre_press_run(void *o)
{
    ARG_UNUSED(o);
}

void pre_press_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* root/fill/pre_press/idle: Idle during pre-pressurization.                */
/* ======================================================================== */
void pre_press_idle_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.filling_state = PRE_PRESS_IDLE;
    close_all_valves(s);
}

void pre_press_idle_run(void *o)
{
    // Transition to vent or fill based on pressure.
    struct sm_object *s = (struct sm_object *)o;
    if (s->data.pressures.n2o_tank_pressure >
        s->config->filling_sm_config.pre_press.trigger_n2o_tank_pressure)
    {
        SET_FILL_STATE(s, PRE_PRESS_VENT);
        return;
    }
    if (s->data.pressures.n2o_tank_pressure <
        s->config->filling_sm_config.pre_press.target_n2o_tank_pressure)
    {
        SET_FILL_STATE(s, PRE_PRESS_FILL_N2);
    }
}

void pre_press_idle_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* root/fill/pre_press/vent: Venting during pre-pressurization.             */
/* ======================================================================== */
void pre_press_vent_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.filling_state = PRE_PRESS_VENT;
    open_single_valve(s, VALVE_VENT);
}

void pre_press_vent_run(void *o)
{
    // Transition to idle when pressure is safe.
    struct sm_object *s = (struct sm_object *)o;
    if (s->data.pressures.n2o_tank_pressure <=
        s->config->filling_sm_config.pre_press.target_n2o_tank_pressure)
    {
        SET_FILL_STATE(s, PRE_PRESS_IDLE);
    }
}

void pre_press_vent_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* root/fill/pre_press/fill_n2: Filling N2 during pre-pressurization.       */
/* ======================================================================== */
void pre_press_fill_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.filling_state = PRE_PRESS_FILL_N2;
    open_single_valve(s, VALVE_PRESSURIZING);
}

void pre_press_fill_run(void *o)
{
    // Transition to idle when pressure reaches target.
    struct sm_object *s = (struct sm_object *)o;

    if (s->data.pressures.n2o_tank_pressure >=
        s->config->filling_sm_config.pre_press.target_n2o_tank_pressure)
    {
        SET_FILL_STATE(s, PRE_PRESS_IDLE);
    }
}

void pre_press_fill_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* root/fill/fill_n2o: N2O filling sequence.                                */
/* ======================================================================== */
void fill_n2o_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.filling_state = FILL_N2O;
    close_all_valves(s);
}

void fill_n2o_run(void *o)
{
    ARG_UNUSED(o);
}

void fill_n2o_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* root/fill/fill_n2o/idle: Idle during N2O filling.                        */
/* ======================================================================== */
void fill_n2o_idle_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.filling_state = FILL_N2O_IDLE;
    close_all_valves(s);
}

void fill_n2o_idle_run(void *o)
{
    // Transition to fill when tank weight is below target.
    struct sm_object *s = (struct sm_object *)o;

    if (s->data.loadcells.n2o_loadcell <
        s->config->filling_sm_config.fill_n2o.target_n2o_tank_weight)
    {
        SET_FILL_STATE(s, FILL_N2O_FILL);
    }
}

void fill_n2o_idle_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* root/fill/fill_n2o/fill: Filling N2O tank.                               */
/* ======================================================================== */
void fill_n2o_fill_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.filling_state = FILL_N2O_FILL;
    open_single_valve(s, VALVE_N2O_FILL);
}

void fill_n2o_fill_run(void *o)
{
    // FIXME: should be minimum temp thermocouple not specific one
    // Transition to vent if pressure and temperature exceed trigger,
    // or to idle if weight reaches target.
    struct sm_object *s = (struct sm_object *)o;

    if (s->data.pressures.n2o_tank_pressure >=
            s->config->filling_sm_config.fill_n2o.trigger_n2o_tank_pressure &&
        s->data.thermocouples.n2o_tank_uf_t1 >
            s->config->filling_sm_config.fill_n2o.trigger_n2o_tank_temperature)
    {
        SET_FILL_STATE(s, FILL_N2O_VENT);
    }
    else if (s->data.loadcells.n2o_loadcell >=
             s->config->filling_sm_config.fill_n2o.target_n2o_tank_weight)
    {
        SET_FILL_STATE(s, FILL_N2O_IDLE);
    }
}

void fill_n2o_fill_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* root/fill/fill_n2o/vent: Venting during N2O filling.                     */
/* ======================================================================== */
void fill_n2o_vent_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.filling_state = FILL_N2O_VENT;
    close_all_valves(s);
    set_valve(s, VALVE_N2O_FILL, true);
    set_valve(s, VALVE_VENT, true);
}

void fill_n2o_vent_run(void *o)
{
    // Transition to fill when pressure or temperature drop below trigger.
    // FIXME: Instead of specific thermocouple should be lowest temp one.
    struct sm_object *s = (struct sm_object *)o;

    if (s->data.pressures.n2o_tank_pressure <=
            s->config->filling_sm_config.fill_n2o.target_n2o_tank_pressure ||
        s->data.thermocouples.n2o_tank_uf_t1 <=
            s->config->filling_sm_config.fill_n2o.trigger_n2o_tank_temperature)
    {
        SET_FILL_STATE(s, FILL_N2O_FILL);
    }
}

void fill_n2o_vent_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* root/fill/post_press: Post-pressurization sequence.                      */
/* ======================================================================== */
void post_press_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    close_all_valves(s);
}

void post_press_run(void *o)
{
    ARG_UNUSED(o);
}

void post_press_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* root/fill/post_press/idle: Idle during post-pressurization.              */
/* ======================================================================== */
void post_press_idle_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.filling_state = POST_PRESS_IDLE;
    close_all_valves(s);
}

void post_press_idle_run(void *o)
{
    // Transition to vent or fill based on pressure.
    struct sm_object *s = (struct sm_object *)o;

    if (s->data.pressures.n2o_tank_pressure >
        s->config->filling_sm_config.post_press.trigger_n2o_tank_pressure)
    {
        SET_FILL_STATE(s, POST_PRESS_VENT);
    }
    else if (s->data.pressures.n2o_tank_pressure <
             s->config->filling_sm_config.post_press.target_n2o_tank_pressure)
    {
        SET_FILL_STATE(s, POST_PRESS_FILL_N2);
    }
}

void post_press_idle_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* root/fill/post_press/fill_n2: Filling N2 during post-pressurization.     */
/* ======================================================================== */
void post_press_fill_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.filling_state = POST_PRESS_FILL_N2;
    open_single_valve(s, VALVE_N2_FILL);
}

void post_press_fill_run(void *o)
{
    // Transition to idle when pressure reaches target.
    struct sm_object *s = (struct sm_object *)o;

    if (s->data.pressures.n2o_tank_pressure >=
        s->config->filling_sm_config.post_press.target_n2o_tank_pressure)
    {
        SET_FILL_STATE(s, POST_PRESS_IDLE);
    }
}

void post_press_fill_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* root/fill/post_press/vent: Venting during post-pressurization.           */
/* ======================================================================== */
void post_press_vent_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.filling_state = POST_PRESS_VENT;
    open_single_valve(s, VALVE_VENT);
}

void post_press_vent_run(void *o)
{
    // Transition to idle when pressure is safe.
    struct sm_object *s = (struct sm_object *)o;

    if (s->data.pressures.n2o_tank_pressure <=
        s->config->filling_sm_config.post_press.target_n2o_tank_pressure)
    {
        SET_FILL_STATE(s, POST_PRESS_IDLE);
    }
}

void post_press_vent_exit(void *o)
{
    ARG_UNUSED(o);
}
