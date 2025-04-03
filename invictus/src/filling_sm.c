#include "filling_sm.h"

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/smf.h>

LOG_MODULE_REGISTER(filling_sm, LOG_LEVEL_DBG);

/* Forward declaration of state table */
#ifndef UNIT_TEST
static const struct smf_state filling_states[];
#endif

static bool transition_global(struct filling_sm_object *s)
{
	enum cmd_global cmd = CMD_GLOBAL(s->command);
	if (!cmd) {
		return false;
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
	}

	return true;
}

/* State Callbacks */

static void idle_entry(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	(void)s; // unused
	LOG_DBG("Entered IDLE state");
	// Do something
	// ...
}

static void idle_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running IDLE state");

	// Do something
	// ...

	if (transition_global(s)) {
		return;
	}

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
	}
}

static void idle_exit(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	(void)s;
	LOG_DBG("Exiting IDLE state");
	// Do something
	// ...
}

static void abort_entry(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	(void)s; // unused
	LOG_DBG("Entered ABORT state");
	// Do something
	// ...
}

static void abort_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running ABORT state");

	// Do something
	// ...

	if (transition_global(s)) {
		return;
	}

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
	}
}

static void abort_exit(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	(void)s; // unused
	LOG_DBG("Exiting ABORT state");
	// Do something
	// ...
}

static void safe_pause_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running SAFE_PAUSE state");

	// Do something
	// ...

	if (transition_global(s)) {
		return;
	}

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
	}
}

static void safe_pause_idle_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running SAFE_PAUSE_IDLE state");

	// Do something
	// ...

	if (transition_global(s)) {
		return;
	}

	if (s->n_pressure > s->s_p_config.trigger_np) {
		LOG_DBG("SAFE_PAUSE_IDLE: n_pressure (%d) > trigger_np (%d) -> SAFE_PAUSE_VENT",
			s->n_pressure, s->s_p_config.trigger_np);
		smf_set_state(SMF_CTX(s), &filling_states[SAFE_PAUSE_VENT]);
	}
}

static void safe_pause_vent_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running SAFE_PAUSE_VENT state");

	// Do something
	// ...

	if (transition_global(s)) {
		return;
	}

	if (s->n_pressure <= s->s_p_config.target_np) {
		LOG_DBG("SAFE_PAUSE_VENT: n_pressure (%d) <= target_np (%d) -> SAFE_PAUSE_IDLE",
			s->n_pressure, s->s_p_config.target_np);
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

static void filling_copv_idle_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running FILLING_COPV_IDLE state");

	// Do something
	// ...

	if (transition_global(s)) {
		return;
	}

	if (s->n_pressure <= s->f_copv_config.target_np) {
		LOG_DBG("FILLING_COPV_IDLE: n_pressure (%d) <= target_np (%d) -> FILLING_COPV_FILL",
			s->n_pressure, s->f_copv_config.target_np);
		smf_set_state(SMF_CTX(s), &filling_states[FILLING_COPV_FILL]);
	}
}

static void filling_copv_fill_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running FILLING_COPV_FILL state");

	// Do something
	// ...

	if (transition_global(s)) {
		return;
	}

	if (s->n_pressure >= s->f_copv_config.target_np) {
		LOG_DBG("FILLING_COPV_FILL: n_pressure (%d) >= target_np (%d) -> FILLING_COPV_IDLE",
			s->n_pressure, s->f_copv_config.target_np);
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

static void pre_pressurizing_idle_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running PRE_PRESSURIZING_IDLE state");

	// Do something
	// ...

	if (transition_global(s)) {
		return;
	}

	// TODO: Double check this condition
	if (s->n_pressure > s->pre_p_config.trigger_n2op) {
		LOG_DBG("PRE_PRESSURIZING_IDLE: n_pressure (%d) > trigger_n2op (%d) -> "
			"PRE_PRESSURIZING_VENT",
			s->n_pressure, s->pre_p_config.trigger_n2op);
		smf_set_state(SMF_CTX(s), &filling_states[PRE_PRESSURIZING_VENT]);
		return;
	}

	if (s->n2o_pressure < s->pre_p_config.target_n2op) {
		LOG_DBG("PRE_PRESSURIZING_IDLE: n2o_pressure (%d) < target_n2op (%d) -> "
			"PRE_PRESSURIZING_FILL_N",
			s->n2o_pressure, s->pre_p_config.target_n2op);
		smf_set_state(SMF_CTX(s), &filling_states[PRE_PRESSURIZING_FILL_N]);
	}
}

static void pre_pressurizing_fill_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running PRE_PRESSURIZING_FILL_N state");

	// Do something
	// ...

	if (transition_global(s)) {
		return;
	}

	if (s->n2o_pressure >= s->pre_p_config.target_n2op) {
		LOG_DBG("PRE_PRESSURIZING_FILL_N: n2o_pressure (%d) >= target_n2op (%d) -> "
			"PRE_PRESSURIZING_IDLE",
			s->n2o_pressure, s->pre_p_config.target_n2op);
		smf_set_state(SMF_CTX(s), &filling_states[PRE_PRESSURIZING_IDLE]);
	}
}

static void pre_pressurizing_vent_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running PRE_PRESSURIZING_VENT state");

	// Do something
	// ...

	if (transition_global(s)) {
		return;
	}

	// TODO: Double check this condition
	if (s->n_pressure <= s->pre_p_config.target_n2op) {
		LOG_DBG("PRE_PRESSURIZING_VENT: n_pressure (%d) <= target_n2op (%d) -> "
			"PRE_PRESSURIZING_IDLE",
			s->n_pressure, s->pre_p_config.target_n2op);
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

static void filling_n20_idle_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running FILLING_N20_IDLE state");

	// Do something
	// ...

	if (transition_global(s)) {
		return;
	}

	if (s->n2o_weight < s->f_n20_config.target_weight) {
		LOG_DBG("FILLING_N20_IDLE: n2o_weight (%d) < target_weight (%d) -> "
			"FILLING_N20_FILL",
			s->n2o_weight, s->f_n20_config.target_weight);
		smf_set_state(SMF_CTX(s), &filling_states[FILLING_N20_FILL]);
	}
}

static void filling_n20_fill_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running FILLING_N20_FILL state");

	// Do something
	// ...

	if (transition_global(s)) {
		return;
	}

	if (s->n2o_pressure >= s->f_n20_config.trigger_n2op &&
	    s->temperature > s->f_n20_config.trigger_temp) {
		LOG_DBG("FILLING_N20_FILL: Conditions met (n2o_pressure=%d, temperature=%d) -> "
			"FILLING_N20_VENT",
			s->n2o_pressure, s->temperature);
		smf_set_state(SMF_CTX(s), &filling_states[FILLING_N20_VENT]);
		return;
	}

	if (s->n2o_weight >= s->f_n20_config.target_weight) {
		LOG_DBG("FILLING_N20_FILL: n2o_weight (%d) >= target_weight (%d) -> "
			"FILLING_N20_IDLE",
			s->n2o_weight, s->f_n20_config.target_weight);
		smf_set_state(SMF_CTX(s), &filling_states[FILLING_N20_IDLE]);
	}
}

static void filling_n20_vent_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running FILLING_N20_VENT state");

	// Do something
	// ...

	if (transition_global(s)) {
		return;
	}

	if (s->n2o_pressure <= s->f_n20_config.target_n2op ||
	    s->temperature <= s->f_n20_config.trigger_temp) {
		LOG_DBG("FILLING_N20_VENT: Conditions met (n2o_pressure=%d, temperature=%d) -> "
			"FILLING_N20_FILL",
			s->n2o_pressure, s->temperature);
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

static void post_pressurizing_idle_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running POST_PRESSURIZING_IDLE state");

	// Do something
	// ...

	if (transition_global(s)) {
		return;
	}

	if (s->n_pressure > s->post_p_config.trigger_n2op) {
		LOG_DBG("POST_PRESSURIZING_IDLE: n_pressure (%d) > trigger_n2op (%d) -> "
			"POST_PRESSURIZING_VENT",
			s->n_pressure, s->post_p_config.trigger_n2op);
		smf_set_state(SMF_CTX(s), &filling_states[POST_PRESSURIZING_VENT]);
		return;
	}

	if (s->n2o_pressure < s->post_p_config.target_n2op) {
		LOG_DBG("POST_PRESSURIZING_IDLE: n2o_pressure (%d) < target_n2op (%d) -> "
			"POST_PRESSURIZING_FILL_N",
			s->n2o_pressure, s->post_p_config.target_n2op);
		smf_set_state(SMF_CTX(s), &filling_states[POST_PRESSURIZING_FILL_N]);
	}
}

static void post_pressurizing_fill_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running POST_PRESSURIZING_FILL_N state");

	// Do something
	// ...

	if (transition_global(s)) {
		return;
	}

	if (s->n2o_pressure >= s->post_p_config.target_n2op) {
		LOG_DBG("POST_PRESSURIZING_FILL_N: n2o_pressure (%d) >= target_n2op (%d) -> "
			"POST_PRESSURIZING_IDLE",
			s->n2o_pressure, s->post_p_config.target_n2op);
		smf_set_state(SMF_CTX(s), &filling_states[POST_PRESSURIZING_IDLE]);
	}
}

static void post_pressurizing_vent_run(void *o)
{
	struct filling_sm_object *s = (struct filling_sm_object *)o;
	LOG_DBG("Running POST_PRESSURIZING_VENT state");

	// Do something
	// ...

	if (transition_global(s)) {
		return;
	}

	if (s->n_pressure <= s->post_p_config.target_n2op) {
		LOG_DBG("POST_PRESSURIZING_VENT: n_pressure (%d) <= target_n2op (%d) -> "
			"POST_PRESSURIZING_IDLE",
			s->n_pressure, s->post_p_config.target_n2op);
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
    [IDLE]      = SMF_CREATE_STATE(idle_entry, idle_run, idle_exit, NULL, NULL),
    [ABORT]     = SMF_CREATE_STATE(abort_entry, abort_run, abort_exit, NULL, NULL),
    [MANUAL_OP] = SMF_CREATE_STATE(NULL, NULL, NULL, NULL, NULL), // TODO

    [SAFE_PAUSE]        = SMF_CREATE_STATE(NULL, safe_pause_run, NULL, NULL, &filling_states[SAFE_PAUSE_IDLE]),
    [SAFE_PAUSE_IDLE]   = SMF_CREATE_STATE(NULL, safe_pause_idle_run, NULL, &filling_states[SAFE_PAUSE], NULL),
    [SAFE_PAUSE_VENT]   = SMF_CREATE_STATE(NULL, safe_pause_vent_run, NULL, &filling_states[SAFE_PAUSE], NULL),

    [FILLING_COPV]      = SMF_CREATE_STATE(NULL, filling_copv_run, NULL, NULL, &filling_states[FILLING_COPV_IDLE]),
    [FILLING_COPV_IDLE] = SMF_CREATE_STATE(NULL, filling_copv_idle_run, NULL, &filling_states[FILLING_COPV], NULL),
    [FILLING_COPV_FILL] = SMF_CREATE_STATE(NULL, filling_copv_fill_run, NULL, &filling_states[FILLING_COPV], NULL),

    [PRE_PRESSURIZING]          = SMF_CREATE_STATE(NULL, pre_pressurizing_run, NULL, NULL, &filling_states[PRE_PRESSURIZING_IDLE]),
    [PRE_PRESSURIZING_IDLE]     = SMF_CREATE_STATE(NULL, pre_pressurizing_idle_run, NULL, &filling_states[PRE_PRESSURIZING], NULL),
    [PRE_PRESSURIZING_VENT]     = SMF_CREATE_STATE(NULL, pre_pressurizing_vent_run, NULL, &filling_states[PRE_PRESSURIZING], NULL),
    [PRE_PRESSURIZING_FILL_N]   = SMF_CREATE_STATE(NULL, pre_pressurizing_fill_run, NULL, &filling_states[PRE_PRESSURIZING], NULL),

    [FILLING_N20]       = SMF_CREATE_STATE(NULL, filling_n20_run, NULL, NULL, &filling_states[FILLING_N20_IDLE]),
    [FILLING_N20_IDLE]  = SMF_CREATE_STATE(NULL, filling_n20_idle_run, NULL, &filling_states[FILLING_N20], NULL),
    [FILLING_N20_FILL]  = SMF_CREATE_STATE(NULL, filling_n20_fill_run, NULL, &filling_states[FILLING_N20], NULL),
    [FILLING_N20_VENT]  = SMF_CREATE_STATE(NULL, filling_n20_vent_run, NULL, &filling_states[FILLING_N20], NULL),

    [POST_PRESSURIZING]         = SMF_CREATE_STATE(NULL, post_pressurizing_run, NULL, NULL, &filling_states[POST_PRESSURIZING_IDLE]),
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
