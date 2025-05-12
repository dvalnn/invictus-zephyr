#include "filling_sm.h"

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
	LOG_DBG("Running ROOT state");

	enum cmd_global cmd = CMD_GLOBAL(s->command);
	if (!cmd) {
		return;
	}

	switch (cmd) {
	case CMD_STOP:
		LOG_DBG("Global transition: CMD_STOP -> IDLE");
		smf_set_state(SMF_CTX(s), &filling_states[IDLE]);
		break;

	case CMD_ABORT:
		LOG_DBG("Global transition: CMD_ABORT -> ABORT");
		smf_set_state(SMF_CTX(s), &filling_states[ABORT]);
		break;

	case CMD_PAUSE:
		LOG_DBG("Global transition: CMD_PAUSE -> SAFE_PAUSE");
		smf_set_state(SMF_CTX(s), &filling_states[SAFE_PAUSE]);
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
	(void)s; // unused
	LOG_DBG("Entered IDLE state");
	// Close all valves
	s->valve_states = 0;
}

static void idle_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running IDLE state");

	enum cmd_idle cmd = CMD_IDLE(s->command);
	if (!cmd) {
		return;
	}

	switch (cmd) {
	case CMD_FILL_COPV:
		LOG_DBG("IDLE state: CMD_FILL_COPV -> FILLING_COPV");
		smf_set_state(SMF_CTX(s), &filling_states[FILLING_COPV]);
		break;
	case CMD_PRE_PRESSURIZE:
		LOG_DBG("IDLE state: CMD_PRE_PRESSURIZE -> PRE_PRESSURIZING");
		smf_set_state(SMF_CTX(s), &filling_states[PRE_PRESSURIZING]);
		break;
	case CMD_FILL_N20:
		LOG_DBG("IDLE state: CMD_FILL_N20 -> FILLING_N20");
		smf_set_state(SMF_CTX(s), &filling_states[FILLING_N20]);
		break;
	case CMD_POST_PRESSURIZE:
		LOG_DBG("IDLE state: CMD_POST_PRESSURIZE -> POST_PRESSURIZING");
		smf_set_state(SMF_CTX(s), &filling_states[POST_PRESSURIZING]);
		break;

	default:
		LOG_ERR("Unknown idle command: %d", cmd);
		smf_set_handled(SMF_CTX(s));
	}

	cmd = 0; // Clear command
}

static void idle_exit(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	(void)s;
	LOG_DBG("Exiting IDLE state");
}

static void abort_entry(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	(void)s; // unused
	LOG_DBG("Entered ABORT state");
	// TODO: 
	// Wait for X seconds
	// Open abort and tank top valves
	s->valve_states = 0 | BIT(VALVE_ABORT) | BIT(VALVE_TANK_TOP);
}

static void abort_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running ABORT state");

	enum cmd_other cmd = CMD_OTHER(s->command);
	if (!cmd) {
		return;
	}

	switch (cmd) {
	case CMD_READY:
		LOG_DBG("ABORT state: CMD_READY -> IDLE");
		smf_set_state(SMF_CTX(s), &filling_states[IDLE]);
		break;

	case CMD_RESUME:
		LOG_DBG("ABORT state: CMD_RESUME (no state change)");
		break;

	default:
		LOG_ERR("Unknown OTHER command: %d", cmd);
		smf_set_handled(SMF_CTX(s));
	}

	cmd = 0; // Clear command
}

static void abort_exit(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	(void)s; // unused
	LOG_DBG("Exiting ABORT state");
}

static void safe_pause_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running SAFE_PAUSE state");

	// Do something
	// ...

	enum cmd_other cmd = CMD_OTHER(s->command);
	if (!cmd) {
		return;
	}

	switch (cmd) {
	case CMD_READY:
		LOG_DBG("SAFE_PAUSE state: CMD_READY (no state change)");
		break;

	case CMD_RESUME:
		LOG_DBG("SAFE_PAUSE state: CMD_RESUME -> IDLE");
		smf_set_state(SMF_CTX(s), &filling_states[IDLE]);
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
	(void)s; // unused
	LOG_DBG("Entered SAFE_PAUSE_IDLE state");
	// Close all valves
	s->valve_states = 0;
}

static void safe_pause_idle_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running SAFE_PAUSE_IDLE state");

	// Do something
	// ...

	if (CMD_GLOBAL(s->command)) {
		return; // Global command takes precedence, do not check conditions
	}

	if (s->data.pre_tank_pressure > s->safe_pause_config.trigger_pre_tank_pressure) {
		const char *var_name = STRINGIFY(s->data.pre_tank_pressure);
		const char *cond_name = STRINGIFY(s->safe_pause_config.trigger_pre_tank_pressure);
		LOG_DBG("SAFE_PAUSE_IDLE: %s (%d) > %s (%d) -> SAFE_PAUSE_VENT", var_name,
			s->data.pre_tank_pressure, cond_name,
			s->safe_pause_config.trigger_pre_tank_pressure);
		smf_set_state(SMF_CTX(s), &filling_states[SAFE_PAUSE_VENT]);
	}
}

static void safe_pause_vent_entry(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	(void)s; // unused
	LOG_DBG("Entered SAFE_PAUSE_VENT state");
	// Open vent valve
	s->valve_states = 0 | BIT(VALVE_VENT);
}

static void safe_pause_vent_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running SAFE_PAUSE_VENT state");

	if (CMD_GLOBAL(s->command)) {
		return; // Global command takes precedence, do not check conditions
	}

	if (s->data.pre_tank_pressure <= s->safe_pause_config.target_pre_tank_pressure) {
		LOG_DBG("SAFE_PAUSE_VENT: data.pre_tank_pressure (%d) <= target_np (%d) -> "
			"SAFE_PAUSE_IDLE",
			s->data.pre_tank_pressure, s->safe_pause_config.target_pre_tank_pressure);
		smf_set_state(SMF_CTX(s), &filling_states[SAFE_PAUSE_IDLE]);
	}
}

static void filling_copv_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	(void)s;
	LOG_DBG("Running FILLING_COPV state");
	// Do something
	// ...
}

static void filling_copv_idle_entry(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	(void)s; // unused
	LOG_DBG("Entered FILLING_COPV_IDLE state");
	// Close all valves
	s->valve_states = 0;
}

static void filling_copv_idle_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running FILLING_COPV_IDLE state");

	// Do something
	// ...

	if (CMD_GLOBAL(s->command)) {
		return; // Global command takes precedence, do not check conditions
	}

	if (s->data.pre_tank_pressure <= s->f_copv_config.target_pre_tank_pressure) {
		const char *var_name = STRINGIFY(s->data.pre_tank_pressure);
		const char *cond_name = STRINGIFY(s->f_copv_config.target_pre_tank_pressure);
		LOG_DBG("FILLING_COPV_IDLE: %s (%d) <= %s (%d) -> FILLING_COPV_FILL", var_name,
			s->data.pre_tank_pressure, cond_name,
			s->f_copv_config.target_pre_tank_pressure);

		smf_set_state(SMF_CTX(s), &filling_states[FILLING_COPV_FILL]);
	}
}

static void filling_copv_fill_entry(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	(void)s; // unused
	LOG_DBG("Entered FILLING_COPV_FILL state");
	// Open N fill valve
	s->valve_states = 0 | BIT(VALVE_N_FILL);
}

static void filling_copv_fill_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running FILLING_COPV_FILL state");

	// Do something
	// ...

	if (CMD_GLOBAL(s->command)) {
		return; // Global command takes precedence, do not check conditions
	}

	if (s->data.pre_tank_pressure >= s->f_copv_config.target_pre_tank_pressure) {
		const char *var_name = STRINGIFY(s->data.pre_tank_pressure);
		const char *cond_name = STRINGIFY(s->f_copv_config.target_pre_tank_pressure);

		LOG_DBG("FILLING_COPV_FILL: %s (%d) >= %s (%d) -> FILLING_COPV_IDLE", var_name,
			s->data.pre_tank_pressure, cond_name,
			s->f_copv_config.target_pre_tank_pressure);

		smf_set_state(SMF_CTX(s), &filling_states[FILLING_COPV_IDLE]);
	}
}

static void pre_pressurizing_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	(void)s;
	LOG_DBG("Running PRE_PRESSURIZING state");
	// Do something
	// ...
}

static void pre_pressurizing_idle_entry(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	(void)s; // unused
	LOG_DBG("Entered PRE_PRESSURIZING_IDLE state");
	// Close all valves
	s->valve_states = 0;
}

static void pre_pressurizing_idle_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running PRE_PRESSURIZING_IDLE state");

	// Do something
	// ...

	if (CMD_GLOBAL(s->command)) {
		return; // Global command takes precedence, do not check conditions
	}

	// TODO: Double check this condition
	if (s->data.pre_tank_pressure > s->pre_p_config.trigger_main_pressure) {
		const char *var_name = STRINGIFY(s->data.pre_tank_pressure);
		const char *cond_name = STRINGIFY(s->pre_p_config.trigger_main_pressure);
		LOG_DBG("PRE_PRESSURIZING_IDLE: %s (%d) > %s (%d) -> PRE_PRESSURIZING_VENT",
			var_name, s->data.pre_tank_pressure, cond_name,
			s->pre_p_config.trigger_main_pressure);

		smf_set_state(SMF_CTX(s), &filling_states[PRE_PRESSURIZING_VENT]);
		return;
	}

	if (s->data.main_tank_pressure < s->pre_p_config.target_main_pressure) {
		const char *var_name = STRINGIFY(s->data.main_tank_pressure);
		const char *cond_name = STRINGIFY(s->pre_p_config.target_main_pressure);
		LOG_DBG("PRE_PRESSURIZING_IDLE: %s (%d) < %s (%d) ->  PRE_PRESSURIZING_FILL_N",
			var_name, s->data.main_tank_pressure, cond_name,
			s->pre_p_config.target_main_pressure);

		smf_set_state(SMF_CTX(s), &filling_states[PRE_PRESSURIZING_FILL_N]);
	}
}

static void pre_pressurizing_fill_entry(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	(void)s; // unused
	LOG_DBG("Entered PRE_PRESSURIZING_FILL_N state");
	// Open N fill valve
	s->valve_states = 0 | BIT(VALVE_TANK_TOP) | BIT(VALVE_N_FILL);
}

static void pre_pressurizing_fill_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running PRE_PRESSURIZING_FILL_N state");

	// Do something
	// ...

	if (CMD_GLOBAL(s->command)) {
		return; // Global command takes precedence, do not check conditions
	}

	if (s->data.main_tank_pressure >= s->pre_p_config.target_main_pressure) {
		const char *var_name = STRINGIFY(s->data.main_tank_pressure);
		const char *cond_name = STRINGIFY(s->pre_p_config.target_main_pressure);
		LOG_DBG("PRE_PRESSURIZING_FILL_N: %s (%d) >= %s (%d) -> PRE_PRESSURIZING_IDLE",
			var_name, s->data.main_tank_pressure, cond_name,
			s->pre_p_config.target_main_pressure);

		smf_set_state(SMF_CTX(s), &filling_states[PRE_PRESSURIZING_IDLE]);
	}
}

static void pre_pressurizing_vent_entry(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	(void)s; // unused
	LOG_DBG("Entered PRE_PRESSURIZING_VENT state");
	// Open vent valve
	s->valve_states = 0 | BIT(VALVE_VENT);
}

static void pre_pressurizing_vent_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running PRE_PRESSURIZING_VENT state");

	// Do something
	// ...

	if (CMD_GLOBAL(s->command)) {
		return; // Global command takes precedence, do not check conditions
	}

	// TODO: Double check this condition
	if (s->data.pre_tank_pressure <= s->pre_p_config.target_main_pressure) {
		const char *var_name = STRINGIFY(s->data.pre_tank_pressure);
		const char *cond_name = STRINGIFY(s->pre_p_config.target_main_pressure);
		LOG_DBG("PRE_PRESSURIZING_VENT: %s (%d) <= %s (%d) -> PRE_PRESSURIZING_IDLE",
			var_name, s->data.pre_tank_pressure, cond_name,
			s->pre_p_config.target_main_pressure);

		smf_set_state(SMF_CTX(s), &filling_states[PRE_PRESSURIZING_IDLE]);
	}
}

static void filling_n20_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	(void)s;
	LOG_DBG("Running FILLING_N20 state");
	// Do something
	// ...
}

static void filling_n20_idle_entry(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	(void)s; // unused
	LOG_DBG("Entered FILLING_N20_IDLE state");
	// Close all valves
	s->valve_states = 0;
}

static void filling_n20_idle_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running FILLING_N20_IDLE state");

	// Do something
	// ...

	if (CMD_GLOBAL(s->command)) {
		return; // Global command takes precedence, do not check conditions
	}

	if (s->data.main_tank_weight < s->f_n20_config.target_main_weight) {
		const char *var_name = STRINGIFY(s->data.main_tank_weight);
		const char *cond_name = STRINGIFY(s->f_n20_config.target_main_weight);
		LOG_DBG("FILLING_N20_IDLE: %s (%d) < %s (%d) -> FILLING_N20_FILL", var_name,
			s->data.main_tank_weight, cond_name, s->f_n20_config.target_main_weight);

		smf_set_state(SMF_CTX(s), &filling_states[FILLING_N20_FILL]);
	}
}

static void filling_n20_fill_entry(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	(void)s; // unused
	LOG_DBG("Entered FILLING_N20_FILL state");
	// Open N20 fill valve
	s->valve_states = 0 | BIT(VALVE_N2O_FILL);
}

static void filling_n20_fill_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running FILLING_N20_FILL state");

	// Do something
	// ...

	if (CMD_GLOBAL(s->command)) {
		return; // Global command takes precedence, do not check conditions
	}

	if (s->data.main_tank_pressure >= s->f_n20_config.trigger_main_pressure &&
	    s->data.main_tank_temperature > s->f_n20_config.trigger_main_temp) {
		const char *var_name = STRINGIFY(s->data.main_tank_pressure);
		const char *cond_name = STRINGIFY(s->f_n20_config.trigger_main_pressure);

		const char *var2_name = STRINGIFY(s->data.main_tank_temperature);
		const char *cond2_name = STRINGIFY(s->f_n20_config.trigger_main_temp);

		LOG_DBG("FILLING_N20_FILL: %s (%d) >= %s (%d) && %s (%d) > %s (%d) ->  "
			"FILLING_N20_VENT",
			var_name, s->data.main_tank_pressure, cond_name,
			s->f_n20_config.trigger_main_pressure, var2_name,
			s->data.main_tank_temperature, cond2_name,
			s->f_n20_config.trigger_main_temp);

		smf_set_state(SMF_CTX(s), &filling_states[FILLING_N20_VENT]);
		return;
	}

	if (s->data.main_tank_weight >= s->f_n20_config.target_main_weight) {
		const char *var_name = STRINGIFY(s->data.main_tank_weight);
		const char *cond_name = STRINGIFY(s->f_n20_config.target_main_weight);
		LOG_DBG("FILLING_N20_FILL: %s (%d) >= %s (%d) -> FILLING_N20_IDLE", var_name,
			s->data.main_tank_weight, cond_name, s->f_n20_config.target_main_weight);

		smf_set_state(SMF_CTX(s), &filling_states[FILLING_N20_IDLE]);
	}
}

static void filling_n20_vent_entry(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	(void)s; // unused
	LOG_DBG("Entered FILLING_N20_VENT state");
	// Open vent and N2O fill valves
	s->valve_states = 0 | BIT(VALVE_N2O_FILL) | BIT(VALVE_VENT);
}

static void filling_n20_vent_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running FILLING_N20_VENT state");

	// Do something
	// ...

	if (CMD_GLOBAL(s->command)) {
		return; // Global command takes precedence, do not check conditions
	}

	if (s->data.main_tank_pressure <= s->f_n20_config.target_main_weight ||
	    s->data.main_tank_temperature <= s->f_n20_config.trigger_main_temp) {
		const char *var_name = STRINGIFY(s->data.main_tank_pressure);
		const char *cond_name = STRINGIFY(s->f_n20_config.target_main_weight);

		const char *var2_name = STRINGIFY(s->data.main_tank_temperature);
		const char *cond2_name = STRINGIFY(s->f_n20_config.trigger_main_temp);

		LOG_DBG("FILLING_N20_VENT: %s (%d) <= %s (%d) || %s (%d) <= %s (%d) -> "
			"FILLING_N20_IDLE",
			var_name, s->data.main_tank_pressure, cond_name,
			s->f_n20_config.target_main_weight, var2_name,
			s->data.main_tank_temperature, cond2_name,
			s->f_n20_config.trigger_main_temp);

		smf_set_state(SMF_CTX(s), &filling_states[FILLING_N20_FILL]);
	}
}

static void post_pressurizing_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	(void)s;
	LOG_DBG("Running POST_PRESSURIZING state");
	// Do something
	// ...
}

static void post_pressurizing_idle_entry(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	(void)s; // unused
	LOG_DBG("Entered POST_PRESSURIZING_IDLE state");
	// Close all valves
	s->valve_states = 0;
}

static void post_pressurizing_idle_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running POST_PRESSURIZING_IDLE state");

	// Do something
	// ...

	if (CMD_GLOBAL(s->command)) {
		return; // Global command takes precedence, do not check conditions
	}

	if (s->data.pre_tank_pressure > s->post_p_config.trigger_main_pressure) {
		const char *var_name = STRINGIFY(s->data.pre_tank_pressure);
		const char *cond_name = STRINGIFY(s->post_p_config.trigger_main_pressure);

		LOG_DBG("POST_PRESSURIZING_IDLE: %s (%d) > %s (%d) -> POST_PRESSURIZING_VENT",
			var_name, s->data.pre_tank_pressure, cond_name,
			s->post_p_config.trigger_main_pressure);

		smf_set_state(SMF_CTX(s), &filling_states[POST_PRESSURIZING_VENT]);
		return;
	}

	if (s->data.main_tank_pressure < s->post_p_config.target_main_pressure) {
		const char *var_name = STRINGIFY(s->data.main_tank_pressure);
		const char *cond_name = STRINGIFY(s->post_p_config.target_main_pressure);
		LOG_DBG("POST_PRESSURIZING_IDLE: %s (%d) < %s (%d) -> POST_PRESSURIZING_FILL_N",
			var_name, s->data.main_tank_pressure, cond_name,
			s->post_p_config.target_main_pressure);

		smf_set_state(SMF_CTX(s), &filling_states[POST_PRESSURIZING_FILL_N]);
	}
}

static void post_pressurizing_fill_entry(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	(void)s; // unused
	LOG_DBG("Entered POST_PRESSURIZING_FILL_N state");
	// Open N fill valve
	s->valve_states = 0 | BIT(VALVE_N_FILL);
}

static void post_pressurizing_fill_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running POST_PRESSURIZING_FILL_N state");

	// Do something
	// ...

	if (CMD_GLOBAL(s->command)) {
		return; // Global command takes precedence, do not check conditions
	}

	if (s->data.main_tank_pressure >= s->post_p_config.target_main_pressure) {
		const char *var_name = STRINGIFY(s->data.main_tank_pressure);
		const char *cond_name = STRINGIFY(s->post_p_config.target_main_pressure);

		LOG_DBG("POST_PRESSURIZING_FILL_N: %s (%d) >= %s (%d) -> POST_PRESSURIZING_IDLE",
			var_name, s->data.main_tank_pressure, cond_name,
			s->post_p_config.target_main_pressure);

		smf_set_state(SMF_CTX(s), &filling_states[POST_PRESSURIZING_IDLE]);
	}
}

static void post_pressurizing_vent_entry(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	(void)s; // unused
	LOG_DBG("Entered POST_PRESSURIZING_VENT state");
	// Open vent valve
	s->valve_states = 0 | BIT(VALVE_VENT);
}

static void post_pressurizing_vent_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running POST_PRESSURIZING_VENT state");

	// Do something
	// ...

	if (CMD_GLOBAL(s->command)) {
		return; // Global command takes precedence, do not check conditions
	}

	if (s->data.pre_tank_pressure <= s->post_p_config.target_main_pressure) {
		const char *var_name = STRINGIFY(s->data.pre_tank_pressure);
		const char *cond_name = STRINGIFY(s->post_p_config.target_main_pressure);

		LOG_DBG("POST_PRESSURIZING_VENT: %s (%d) <= %s (%d) -> POST_PRESSURIZING_IDLE",
			var_name, s->data.pre_tank_pressure, cond_name,
			s->post_p_config.target_main_pressure);

		smf_set_state(SMF_CTX(s), &filling_states[POST_PRESSURIZING_IDLE]);
	}
}

#ifdef UNIT_TEST
const struct smf_state filling_states[] = {
#else
static const struct smf_state filling_states[] = {
#endif
	// clang-format off
    // SMF_CREATE_STATE(s_entry_cb, s_run_cb, s_exit_cb, s_parent, s_initial_o),
    [ROOT]      = SMF_CREATE_STATE(NULL, root_run, NULL, NULL, &filling_states[IDLE]),
    
    [IDLE]      = SMF_CREATE_STATE(idle_entry, idle_run, idle_exit, &filling_states[ROOT], NULL),
    [ABORT]     = SMF_CREATE_STATE(abort_entry, abort_run, abort_exit,&filling_states[ROOT], NULL),
    [MANUAL_OP] = SMF_CREATE_STATE(NULL, NULL, NULL, NULL, NULL), // TODO

    [SAFE_PAUSE]        = SMF_CREATE_STATE(NULL, safe_pause_run, NULL, &filling_states[ROOT], &filling_states[SAFE_PAUSE_IDLE]),
    [SAFE_PAUSE_IDLE]   = SMF_CREATE_STATE(NULL, safe_pause_idle_run, NULL, &filling_states[SAFE_PAUSE], NULL),
    [SAFE_PAUSE_VENT]   = SMF_CREATE_STATE(NULL, safe_pause_vent_run, NULL, &filling_states[SAFE_PAUSE], NULL),

    [FILLING_COPV]      = SMF_CREATE_STATE(NULL, filling_copv_run, NULL, &filling_states[ROOT], &filling_states[FILLING_COPV_IDLE]),
    [FILLING_COPV_IDLE] = SMF_CREATE_STATE(NULL, filling_copv_idle_run, NULL, &filling_states[FILLING_COPV], NULL),
    [FILLING_COPV_FILL] = SMF_CREATE_STATE(NULL, filling_copv_fill_run, NULL, &filling_states[FILLING_COPV], NULL),

    [PRE_PRESSURIZING]          = SMF_CREATE_STATE(NULL, pre_pressurizing_run, NULL, &filling_states[ROOT], &filling_states[PRE_PRESSURIZING_IDLE]),
    [PRE_PRESSURIZING_IDLE]     = SMF_CREATE_STATE(NULL, pre_pressurizing_idle_run, NULL, &filling_states[PRE_PRESSURIZING], NULL),
    [PRE_PRESSURIZING_VENT]     = SMF_CREATE_STATE(NULL, pre_pressurizing_vent_run, NULL, &filling_states[PRE_PRESSURIZING], NULL),
    [PRE_PRESSURIZING_FILL_N]   = SMF_CREATE_STATE(NULL, pre_pressurizing_fill_run, NULL, &filling_states[PRE_PRESSURIZING], NULL),

    [FILLING_N20]       = SMF_CREATE_STATE(NULL, filling_n20_run, NULL, &filling_states[ROOT], &filling_states[FILLING_N20_IDLE]),
    [FILLING_N20_IDLE]  = SMF_CREATE_STATE(NULL, filling_n20_idle_run, NULL, &filling_states[FILLING_N20], NULL),
    [FILLING_N20_FILL]  = SMF_CREATE_STATE(NULL, filling_n20_fill_run, NULL, &filling_states[FILLING_N20], NULL),
    [FILLING_N20_VENT]  = SMF_CREATE_STATE(NULL, filling_n20_vent_run, NULL, &filling_states[FILLING_N20], NULL),

    [POST_PRESSURIZING]         = SMF_CREATE_STATE(NULL, post_pressurizing_run, NULL, &filling_states[ROOT], &filling_states[POST_PRESSURIZING_IDLE]),
    [POST_PRESSURIZING_IDLE]    = SMF_CREATE_STATE(NULL, post_pressurizing_idle_run, NULL, &filling_states[POST_PRESSURIZING], NULL),
    [POST_PRESSURIZING_VENT]    = SMF_CREATE_STATE(NULL, post_pressurizing_vent_run, NULL, &filling_states[POST_PRESSURIZING], NULL),
    [POST_PRESSURIZING_FILL_N]  = SMF_CREATE_STATE(NULL, post_pressurizing_fill_run, NULL, &filling_states[POST_PRESSURIZING], NULL),
	// clang-format on
};

void filling_sm_init(struct filling_sm_object *initial_s_obj)
{
	LOG_DBG("Initializing state machine: setting initial state to IDLE");
	smf_set_initial(SMF_CTX(initial_s_obj), &filling_states[IDLE]);
}
