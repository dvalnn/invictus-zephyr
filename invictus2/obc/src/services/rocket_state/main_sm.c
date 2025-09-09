#include "services/rocket_state/main_sm.h"

#include "data_models.h"
#include "zephyr/smf.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(filling_sm, LOG_LEVEL_DBG);

#ifndef UNIT_TEST
/* Forward declaration of state table.
 * If unittesting, the state table is exported from the header file instead.
 */
static const struct smf_state filling_states[];
#endif

static void root_run(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Running RS_FILL_ROOT state");

    enum cmd_global cmd = CMD_GLOBAL(s->command);
    if (!cmd) {
        return;
    }

    switch (cmd) {
    case CMD_STOP:
        LOG_DBG("Global transition: CMD_STOP -> RS_FILL_IDLE");
        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_IDLE]);
        break;

    case CMD_ABORT:
        LOG_DBG("Global transition: CMD_RS_FILL_ABORT -> RS_FILL_ABORT");
        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_ABORT]);
        break;

    case CMD_PAUSE:
        LOG_DBG("Global transition: CMD_PAUSE -> RS_FILL_SAFE_PAUSE");
        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_SAFE_PAUSE]);
        break;

    default:
        LOG_ERR("Unknown global command: %d", cmd);
    }

    cmd = 0; // Clear command
}

/* State Callbacks */

static void idle_entry(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Entered RS_FILL_IDLE state");

    s->valve_states.raw = 0;
}

static void idle_run(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Running RS_FILL_IDLE state");

    enum cmd_idle cmd = CMD_IDLE(s->command);
    if (!cmd) {
        return;
    }

    switch (cmd) {
    case CMD_FILL_COPV:
        LOG_DBG("RS_FILL_IDLE state: CMD_FILL_COPV -> RS_FILL_FILLING_N2");
        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_FILLING_N2]);
        break;
    case CMD_PRE_PRESSURIZE:
        LOG_DBG("RS_FILL_IDLE state: CMD_PRE_PRESSURIZE -> RS_FILL_PRE_PRESS");
        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_PRE_PRESS]);
        break;
    case CMD_FILL_N2O:
        LOG_DBG("RS_FILL_IDLE state: CMD_FILL_N2O -> RS_FILL_FILLING_N2O");
        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_FILLING_N2O]);
        break;
    case CMD_POST_PRESSURIZE:
        LOG_DBG("RS_FILL_IDLE state: CMD_POST_PRESSURIZE -> RS_FILL_POST_PRESS");
        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_POST_PRESS]);
        break;

    default:
        LOG_ERR("Unknown idle command: %d", cmd);
        smf_set_handled(SMF_CTX(s));
    }

    cmd = 0; // Clear command
}

static void idle_exit(void *o)
{
    ARG_UNUSED(o);
    LOG_DBG("Exiting RS_FILL_IDLE state");
}

static void abort_entry(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    (void)s; // unused
    LOG_DBG("Entered RS_FILL_ABORT state");
    // TODO:
    // Wait for X seconds
    // Open abort valve
    s->valve_states.raw = 0;
    s->valve_states.abort = 1;
    s->valve_states.pressurizing = 1;
}

static void abort_run(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Running RS_FILL_ABORT state");

    enum cmd_other cmd = CMD_OTHER(s->command);
    if (!cmd) {
        return;
    }

    switch (cmd) {
    case CMD_READY:
        LOG_DBG("RS_FILL_ABORT state: CMD_READY -> RS_FILL_IDLE");
        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_IDLE]);
        break;

    case CMD_RESUME:
        LOG_DBG("RS_FILL_ABORT state: CMD_RESUME (no state change)");
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

static void safe_pause_run(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Running RS_FILL_SAFE_PAUSE state");

    enum cmd_other cmd = CMD_OTHER(s->command);
    if (!cmd) {
        return;
    }

    switch (cmd) {
    case CMD_READY:
        LOG_DBG("RS_FILL_SAFE_PAUSE state: CMD_READY (no state change)");
        break;

    case CMD_RESUME:
        LOG_DBG("RS_FILL_SAFE_PAUSE state: CMD_RESUME -> RS_FILL_IDLE");
        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_IDLE]);
        break;

    default:
        LOG_ERR("Unknown OTHER command: %d", cmd);
        smf_set_handled(SMF_CTX(s));
    }

    cmd = 0; // Clear command
}

static void safe_pause_idle_entry(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Entered RS_FILL_SAFE_PAUSE_IDLE state");

    s->valve_states.raw = 0;
}

static void safe_pause_idle_run(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Running RS_FILL_SAFE_PAUSE_IDLE state");

    if (CMD_GLOBAL(s->command)) {
        return; // Global command takes precedence, do not check conditions
    }

    if (s->data.n2o_tank_pressure > s->config->safe_pause.trigger_n2o_tank_pressure) {
        const char *var_name = STRINGIFY(s->data.n2o_tank_pressure);
        const char *cond_name = STRINGIFY(s->config->safe_pause.trigger_n2o_tank_pressure);
        LOG_DBG("RS_FILL_SAFE_PAUSE_IDLE: %s (%d) > %s (%d) -> RS_FILL_SAFE_PAUSE_VENT",
                var_name, s->data.n2o_tank_pressure, cond_name,
                s->config->safe_pause.trigger_n2o_tank_pressure);
        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_SAFE_PAUSE_VENT]);
    }
}

static void safe_pause_vent_entry(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Entered RS_FILL_SAFE_PAUSE_VENT state");

    s->valve_states.raw = 0;
    s->valve_states.vent = 1;
}

static void safe_pause_vent_run(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Running RS_FILL_SAFE_PAUSE_VENT state");

    if (CMD_GLOBAL(s->command)) {
        return; // Global command takes precedence, do not check conditions
    }

    if (s->data.n2o_tank_pressure <= s->config->safe_pause.target_n2o_tank_pressure) {
        LOG_DBG("RS_FILL_SAFE_PAUSE_VENT: data.n2o_tank_pressure (%d) <= "
                "target_n2o_tank_pressure (%d) -> "
                "RS_FILL_SAFE_PAUSE_IDLE",
                s->data.n2o_tank_pressure, s->config->safe_pause.target_n2o_tank_pressure);
        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_SAFE_PAUSE_IDLE]);
    }
}

static void filling_copv_run(void *o)
{
    ARG_UNUSED(o);
    LOG_DBG("Running RS_FILL_FILLING_N2 state");
}

static void filling_copv_idle_entry(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Entered RS_FILL_FILLING_N2_IDLE state");

    s->valve_states.raw = 0;
}

static void filling_copv_idle_run(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Running RS_FILL_FILLING_N2_IDLE state");

    if (CMD_GLOBAL(s->command)) {
        return; // Global command takes precedence, do not check conditions
    }

    if (s->data.n2_tank_pressure <= s->config->f_copv.target_n2_tank_pressure) {
        const char *var_name = STRINGIFY(s->data.n2_tank_pressure);
        const char *cond_name = STRINGIFY(s->config->f_copv.target_n2_tank_pressure);
        LOG_DBG("RS_FILL_FILLING_N2_IDLE: %s (%d) <= %s (%d) -> RS_FILL_FILLING_N2_FILL",
                var_name, s->data.n2_tank_pressure, cond_name,
                s->config->f_copv.target_n2_tank_pressure);

        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_FILLING_N2_FILL]);
    }
}

static void filling_copv_fill_entry(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    (void)s; // unused
    LOG_DBG("Entered RS_FILL_FILLING_N2_FILL state");

    s->valve_states.raw = 0;
    s->valve_states.n2_fill = 1;
}

static void filling_copv_fill_run(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Running RS_FILL_FILLING_N2_FILL state");

    if (CMD_GLOBAL(s->command)) {
        return; // Global command takes precedence, do not check conditions
    }

    if (s->data.n2_tank_pressure >= s->config->f_copv.target_n2_tank_pressure) {
        const char *var_name = STRINGIFY(s->data.n2_tank_pressure);
        const char *cond_name = STRINGIFY(s->config->f_copv.target_n2_tank_pressure);

        LOG_DBG("RS_FILL_FILLING_N2_FILL: %s (%d) >= %s (%d) -> RS_FILL_FILLING_N2_IDLE",
                var_name, s->data.n2_tank_pressure, cond_name,
                s->config->f_copv.target_n2_tank_pressure);

        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_FILLING_N2_IDLE]);
    }
}

static void pre_press_run(void *o)
{
    ARG_UNUSED(o);
    LOG_DBG("Running RS_FILL_PRE_PRESS state");
}

static void pre_press_idle_entry(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Entered RS_FILL_PRE_PRESS_IDLE state");

    s->valve_states.raw = 0;
}

static void pre_press_idle_run(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Running RS_FILL_PRE_PRESS_IDLE state");

    if (CMD_GLOBAL(s->command)) {
        return; // Global command takes precedence, do not check conditions
    }

    // TODO: Double check this condition
    if (s->data.n2o_tank_pressure > s->config->pre_p.trigger_n2o_tank_pressure) {
        const char *var_name = STRINGIFY(s->data.n2o_tank_pressure);
        const char *cond_name = STRINGIFY(s->config->pre_p.trigger_n2o_tank_pressure);
        LOG_DBG("RS_FILL_PRE_PRESS_IDLE: %s (%d) > %s (%d) -> RS_FILL_PRE_PRESS_VENT",
                var_name, s->data.n2o_tank_pressure, cond_name,
                s->config->pre_p.trigger_n2o_tank_pressure);

        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_PRE_PRESS_VENT]);
        return;
    }

    if (s->data.n2o_tank_pressure < s->config->pre_p.target_n2o_tank_pressure) {
        const char *var_name = STRINGIFY(s->data.n2o_tank_pressure);
        const char *cond_name = STRINGIFY(s->config->pre_p.target_n2o_tank_pressure);
        LOG_DBG("RS_FILL_PRE_PRESS_IDLE: %s (%d) < %s (%d) ->  "
                "RS_FILL_PRE_PRESS_FILL_N2",
                var_name, s->data.n2o_tank_pressure, cond_name,
                s->config->pre_p.target_n2o_tank_pressure);

        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_PRE_PRESS_FILL_N2]);
    }
}

static void pre_press_fill_entry(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Entered RS_FILL_PRE_PRESS_FILL_N2 state");

    s->valve_states.raw = 0;
    s->valve_states.pressurizing = 1;
}

static void pre_press_fill_run(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Running RS_FILL_PRE_PRESS_FILL_N2 state");

    if (CMD_GLOBAL(s->command)) {
        return; // Global command takes precedence, do not check conditions
    }

    if (s->data.n2o_tank_pressure >= s->config->pre_p.target_n2o_tank_pressure) {
        const char *var_name = STRINGIFY(s->data.n2o_tank_pressure);
        const char *cond_name = STRINGIFY(s->config->pre_p.target_n2o_tank_pressure);
        LOG_DBG("RS_FILL_PRE_PRESS_FILL_N2: %s (%d) >= %s (%d) -> "
                "RS_FILL_PRE_PRESS_IDLE",
                var_name, s->data.n2o_tank_pressure, cond_name,
                s->config->pre_p.target_n2o_tank_pressure);

        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_PRE_PRESS_IDLE]);
    }
}

static void pre_press_vent_entry(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Entered RS_FILL_PRE_PRESS_VENT state");

    s->valve_states.raw = 0;
    s->valve_states.vent = 1;
}

static void pre_press_vent_run(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Running RS_FILL_PRE_PRESS_VENT state");

    if (CMD_GLOBAL(s->command)) {
        return; // Global command takes precedence, do not check conditions
    }

    // TODO: Double check this condition
    if (s->data.n2o_tank_pressure <= s->config->pre_p.target_n2o_tank_pressure) {
        const char *var_name = STRINGIFY(s->data.n2o_tank_pressure);
        const char *cond_name = STRINGIFY(s->config->pre_p.target_n2o_tank_pressure);
        LOG_DBG("RS_FILL_PRE_PRESS_VENT: %s (%d) <= %s (%d) -> RS_FILL_PRE_PRESS_IDLE",
                var_name, s->data.n2o_tank_pressure, cond_name,
                s->config->pre_p.target_n2o_tank_pressure);

        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_PRE_PRESS_IDLE]);
    }
}

static void filling_n2o_run(void *o)
{
    ARG_UNUSED(o);
    LOG_DBG("Running RS_FILL_FILLING_N2O state");
}

static void filling_n2o_idle_entry(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Entered RS_FILL_FILLING_N2O_IDLE state");
    s->valve_states.raw = 0;
}

static void filling_n2o_idle_run(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Running RS_FILL_FILLING_N2O_IDLE state");

    if (CMD_GLOBAL(s->command)) {
        return; // Global command takes precedence, do not check conditions
    }

    if (s->data.n2o_tank_weight < s->config->f_n2o.target_n2o_tank_weight) {
        const char *var_name = STRINGIFY(s->data.n2o_tank_weight);
        const char *cond_name = STRINGIFY(s->config->f_n2o.target_n2o_tank_weight);
        LOG_DBG("RS_FILL_FILLING_N2O_IDLE: %s (%d) < %s (%d) -> RS_FILL_FILLING_N2O_FILL",
                var_name, s->data.n2o_tank_weight, cond_name,
                s->config->f_n2o.target_n2o_tank_weight);

        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_FILLING_N2O_FILL]);
    }
}

static void filling_n2o_fill_entry(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Entered RS_FILL_FILLING_N2O_FILL state");

    s->valve_states.raw = 0;
    s->valve_states.n2o_fill = 1;
}

static void filling_n2o_fill_run(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Running RS_FILL_FILLING_N2O_FILL state");

    if (CMD_GLOBAL(s->command)) {
        return; // Global command takes precedence, do not check conditions
    }

    if (s->data.n2o_tank_pressure >= s->config->f_n2o.trigger_n2o_tank_pressure &&
        s->data.n2o_tank_temperature > s->config->f_n2o.trigger_n2o_tank_temperature) {
        const char *var_name = STRINGIFY(s->data.n2o_tank_pressure);
        const char *cond_name = STRINGIFY(s->config->f_n2o.trigger_n2o_tank_pressure);

        const char *var2_name = STRINGIFY(s->data.n2o_tank_temperature);
        const char *cond2_name = STRINGIFY(s->config->f_n2o.trigger_n2o_tank_temperature);

        LOG_DBG("RS_FILL_FILLING_N2O_FILL: %s (%d) >= %s (%d) && %s (%d) > %s (%d) ->  "
                "RS_FILL_FILLING_N2O_VENT",
                var_name, s->data.n2o_tank_pressure, cond_name,
                s->config->f_n2o.trigger_n2o_tank_pressure, var2_name,
                s->data.n2o_tank_temperature, cond2_name,
                s->config->f_n2o.trigger_n2o_tank_temperature);

        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_FILLING_N2O_VENT]);
        return;
    }

    if (s->data.n2o_tank_weight >= s->config->f_n2o.target_n2o_tank_weight) {
        const char *var_name = STRINGIFY(s->data.n2o_tank_weight);
        const char *cond_name = STRINGIFY(s->config->f_n2o.target_n2o_tank_weight);
        LOG_DBG("RS_FILL_FILLING_N2O_FILL: %s (%d) >= %s (%d) -> RS_FILL_FILLING_N2O_IDLE",
                var_name, s->data.n2o_tank_weight, cond_name,
                s->config->f_n2o.target_n2o_tank_weight);

        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_FILLING_N2O_IDLE]);
    }
}

static void filling_n2o_vent_entry(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Entered RS_FILL_FILLING_N2O_VENT state");

    s->valve_states.raw = 0;
    s->valve_states.n2o_fill = 1;
    s->valve_states.vent = 1;
}

static void filling_n2o_vent_run(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Running RS_FILL_FILLING_N2O_VENT state");

    if (CMD_GLOBAL(s->command)) {
        return; // Global command takes precedence, do not check conditions
    }

    if (s->data.n2o_tank_pressure <= s->config->f_n2o.target_n2o_tank_pressure ||
        s->data.n2o_tank_temperature <= s->config->f_n2o.trigger_n2o_tank_temperature) {
        const char *var_name = STRINGIFY(s->data.n2o_tank_pressure);
        const char *cond_name = STRINGIFY(s->config->f_n2o.target_n2o_tank_pressure);

        const char *var2_name = STRINGIFY(s->data.n2o_tank_temperature);
        const char *cond2_name = STRINGIFY(s->config->f_n2o.trigger_n2o_tank_temperature);

        LOG_DBG("RS_FILL_FILLING_N2O_VENT: %s (%d) <= %s (%d) || %s (%d) <= %s (%d) -> "
                "RS_FILL_FILLING_N2O_FILL",
                var_name, s->data.n2o_tank_pressure, cond_name,
                s->config->f_n2o.target_n2o_tank_pressure, var2_name,
                s->data.n2o_tank_temperature, cond2_name,
                s->config->f_n2o.trigger_n2o_tank_temperature);

        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_FILLING_N2O_FILL]);
    }
}

static void post_press_run(void *o)
{
    ARG_UNUSED(o);
    LOG_DBG("Running RS_FILL_POST_PRESS state");
}

static void post_press_idle_entry(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Entered RS_FILL_POST_PRESS_IDLE state");
    s->valve_states.raw = 0;
}

static void post_press_idle_run(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Running RS_FILL_POST_PRESS_IDLE state");

    // Do something
    // ...

    if (CMD_GLOBAL(s->command)) {
        return; // Global command takes precedence, do not check conditions
    }

    if (s->data.n2o_tank_pressure > s->config->post_p.trigger_n2o_tank_pressure) {
        const char *var_name = STRINGIFY(s->data.n2o_tank_pressure);
        const char *cond_name = STRINGIFY(s->config->post_p.trigger_n2o_tank_pressure);

        LOG_DBG("RS_FILL_POST_PRESS_IDLE: %s (%d) > %s (%d) -> RS_FILL_POST_PRESS_VENT",
                var_name, s->data.n2o_tank_pressure, cond_name,
                s->config->post_p.trigger_n2o_tank_pressure);

        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_POST_PRESS_VENT]);
        return;
    }

    if (s->data.n2o_tank_pressure < s->config->post_p.target_n2o_tank_pressure) {
        const char *var_name = STRINGIFY(s->data.n2o_tank_pressure);
        const char *cond_name = STRINGIFY(s->config->post_p.target_n2o_tank_pressure);
        LOG_DBG("RS_FILL_POST_PRESS_IDLE: %s (%d) < %s (%d) -> "
                "RS_FILL_POST_PRESS_FILL_N2",
                var_name, s->data.n2o_tank_pressure, cond_name,
                s->config->post_p.target_n2o_tank_pressure);

        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_POST_PRESS_FILL_N2]);
    }
}

static void post_press_fill_entry(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Entered RS_FILL_POST_PRESS_FILL_N2 state");

    s->valve_states.raw = 0;
    s->valve_states.n2_fill = 1;
}

static void post_press_fill_run(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Running RS_FILL_POST_PRESS_FILL_N2 state");

    if (CMD_GLOBAL(s->command)) {
        return; // Global command takes precedence, do not check conditions
    }

    if (s->data.n2o_tank_pressure >= s->config->post_p.target_n2o_tank_pressure) {
        const char *var_name = STRINGIFY(s->data.n2o_tank_pressure);
        const char *cond_name = STRINGIFY(s->config->post_p.target_n2o_tank_pressure);

        LOG_DBG("RS_FILL_POST_PRESS_FILL_N2: %s (%d) >= %s (%d) -> "
                "RS_FILL_POST_PRESS_IDLE",
                var_name, s->data.n2o_tank_pressure, cond_name,
                s->config->post_p.target_n2o_tank_pressure);

        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_POST_PRESS_IDLE]);
    }
}

static void post_press_vent_entry(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Entered RS_FILL_POST_PRESS_VENT state");

    s->valve_states.raw = 0;
    s->valve_states.vent = 1;
}

static void post_press_vent_run(void *o)
{
    struct filling_sm_object *s = (struct filling_sm_object *)o;
    LOG_DBG("Running RS_FILL_POST_PRESS_VENT state");

    if (CMD_GLOBAL(s->command)) {
        return; // Global command takes precedence, do not check conditions
    }

    if (s->data.n2o_tank_pressure <= s->config->post_p.target_n2o_tank_pressure) {
        const char *var_name = STRINGIFY(s->data.n2o_tank_pressure);
        const char *cond_name = STRINGIFY(s->config->post_p.target_n2o_tank_pressure);

        LOG_DBG("RS_FILL_POST_PRESS_VENT: %s (%d) <= %s (%d) -> "
                "RS_FILL_POST_PRESS_IDLE",
                var_name, s->data.n2o_tank_pressure, cond_name,
                s->config->post_p.target_n2o_tank_pressure);

        smf_set_state(SMF_CTX(s), &filling_states[RS_FILL_POST_PRESS_IDLE]);
    }
}

#ifdef UNIT_TEST
const struct smf_state filling_states[] = {
#else
static const struct smf_state filling_states[] = {
#endif
    // clang-format off
    // SMF_CREATE_STATE(s_entry_cb, s_run_cb, s_exit_cb, s_parent, s_initial_o),
    [RS_FILL_ROOT]      = SMF_CREATE_STATE(NULL, root_run, NULL, NULL, &filling_states[RS_FILL_IDLE]),
    
    [RS_FILL_IDLE]      = SMF_CREATE_STATE(idle_entry, idle_run, idle_exit, &filling_states[RS_FILL_ROOT], NULL),
    [RS_FILL_ABORT]     = SMF_CREATE_STATE(abort_entry, abort_run, abort_exit,&filling_states[RS_FILL_ROOT], NULL),

    [RS_FILL_SAFE_PAUSE]         = SMF_CREATE_STATE(NULL, safe_pause_run, NULL, &filling_states[RS_FILL_ROOT], &filling_states[RS_FILL_SAFE_PAUSE_IDLE]),
    [RS_FILL_SAFE_PAUSE_IDLE]    = SMF_CREATE_STATE(safe_pause_idle_entry, safe_pause_idle_run, NULL, &filling_states[RS_FILL_SAFE_PAUSE], NULL),
    [RS_FILL_SAFE_PAUSE_VENT]    = SMF_CREATE_STATE(safe_pause_vent_entry, safe_pause_vent_run, NULL, &filling_states[RS_FILL_SAFE_PAUSE], NULL),

    [RS_FILL_FILLING_N2]         = SMF_CREATE_STATE(NULL, filling_copv_run, NULL, &filling_states[RS_FILL_ROOT], &filling_states[RS_FILL_FILLING_N2_IDLE]),
    [RS_FILL_FILLING_N2_IDLE]    = SMF_CREATE_STATE(filling_copv_idle_entry, filling_copv_idle_run, NULL, &filling_states[RS_FILL_FILLING_N2], NULL),
    [RS_FILL_FILLING_N2_FILL]    = SMF_CREATE_STATE(filling_copv_fill_entry, filling_copv_fill_run, NULL, &filling_states[RS_FILL_FILLING_N2], NULL),

    [RS_FILL_PRE_PRESS]          = SMF_CREATE_STATE(NULL, pre_press_run, NULL, &filling_states[RS_FILL_ROOT], &filling_states[RS_FILL_PRE_PRESS_IDLE]),
    [RS_FILL_PRE_PRESS_IDLE]     = SMF_CREATE_STATE(pre_press_idle_entry, pre_press_idle_run, NULL, &filling_states[RS_FILL_PRE_PRESS], NULL),
    [RS_FILL_PRE_PRESS_VENT]     = SMF_CREATE_STATE(pre_press_vent_entry, pre_press_vent_run, NULL, &filling_states[RS_FILL_PRE_PRESS], NULL),
    [RS_FILL_PRE_PRESS_FILL_N2]  = SMF_CREATE_STATE(pre_press_fill_entry, pre_press_fill_run, NULL, &filling_states[RS_FILL_PRE_PRESS], NULL),

    [RS_FILL_FILLING_N2O]        = SMF_CREATE_STATE(NULL, filling_n2o_run, NULL, &filling_states[RS_FILL_ROOT], &filling_states[RS_FILL_FILLING_N2O_IDLE]),
    [RS_FILL_FILLING_N2O_IDLE]   = SMF_CREATE_STATE(filling_n2o_idle_entry, filling_n2o_idle_run, NULL, &filling_states[RS_FILL_FILLING_N2O], NULL),
    [RS_FILL_FILLING_N2O_FILL]   = SMF_CREATE_STATE(filling_n2o_fill_entry, filling_n2o_fill_run, NULL, &filling_states[RS_FILL_FILLING_N2O], NULL),
    [RS_FILL_FILLING_N2O_VENT]   = SMF_CREATE_STATE(filling_n2o_vent_entry, filling_n2o_vent_run, NULL, &filling_states[RS_FILL_FILLING_N2O], NULL),

    [RS_FILL_POST_PRESS]         = SMF_CREATE_STATE(NULL, post_press_run, NULL, &filling_states[RS_FILL_ROOT], &filling_states[RS_FILL_POST_PRESS_IDLE]),
    [RS_FILL_POST_PRESS_IDLE]    = SMF_CREATE_STATE(post_press_idle_entry, post_press_idle_run, NULL, &filling_states[RS_FILL_POST_PRESS], NULL),
    [RS_FILL_POST_PRESS_VENT]    = SMF_CREATE_STATE(post_press_vent_entry, post_press_vent_run, NULL, &filling_states[RS_FILL_POST_PRESS], NULL),
    [RS_FILL_POST_PRESS_FILL_N2] = SMF_CREATE_STATE(post_press_fill_entry, post_press_fill_run, NULL, &filling_states[RS_FILL_POST_PRESS], NULL),
    // clang-format on
};

void filling_sm_init(struct filling_sm_object *initial_s_obj)
{
    LOG_DBG("Initializing state machine: setting initial state to RS_FILL_IDLE");
    smf_set_initial(SMF_CTX(initial_s_obj), &filling_states[RS_FILL_IDLE]);
}
