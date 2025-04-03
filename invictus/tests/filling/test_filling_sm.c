#include <zephyr/ztest.h>
#include <zephyr/smf.h>

#include "filling_sm.h"

/* Global test object for the state machine */
static struct filling_sm_object test_obj;

/* Reset the test object with default values */
static void reset_test_obj(void)
{
	memset(&test_obj, 0, sizeof(test_obj));

	/* Set default configuration values for testing */
	test_obj.n_pressure = 0;
	test_obj.n2o_pressure = 0;
	test_obj.n2o_weight = 0;
	test_obj.temperature = 0;
	test_obj.s_p_config.target_np = 100;
	test_obj.s_p_config.trigger_np = 50;
	test_obj.f_copv_config.target_np = 75;
	test_obj.pre_p_config.target_n2op = 80;
	test_obj.pre_p_config.trigger_n2op = 40;
	test_obj.f_n20_config.target_n2op = 90;
	test_obj.f_n20_config.target_weight = 100;
	test_obj.f_n20_config.trigger_n2op = 60;
	test_obj.f_n20_config.trigger_temp = 25;
	test_obj.post_p_config.target_n2op = 85;
	test_obj.post_p_config.trigger_n2op = 45;

	/* Clear any command */
	test_obj.command = 0;
}

ZTEST_SUITE(filling_sm_suite, NULL, NULL, NULL, NULL, NULL);

/* Test: Verify that the initial state is IDLE */
ZTEST(filling_sm_suite, test_initial_state)
{
	reset_test_obj();

	/* Initialize the state machine */
	filling_sm_init(&test_obj);

	/* Check if the initial state is IDLE */
	zassert_equal(test_obj.ctx.current, &filling_states[IDLE], "Initial state is not IDLE");
}

/* Test: Transition from IDLE to FILLING_COPV_IDLE via CMD_FILL_COPV */
ZTEST(filling_sm_suite, test_idle_to_filling_copv)
{
	reset_test_obj();

	/* Initialize the state machine */
	filling_sm_init(&test_obj);

	/* Set command to CMD_FILL_COPV */
	test_obj.command = CMD_FILL_COPV;

	/* Run the state machine */
	smf_run_state(SMF_CTX(&test_obj));

	/* Check if the state has transitioned to FILLING_COPV_IDLE */
	zassert_equal(test_obj.ctx.current, &filling_states[FILLING_COPV_IDLE],
		      "State did not transition to FILLING_COPV_IDLE"
		      "Current state: %p",
		      test_obj.ctx.current);

	test_obj.command = CMD_STOP;

	/* Run the state machine again */
	smf_run_state(SMF_CTX(&test_obj));

	/* Check if the state has transitioned back to IDLE */
	zassert_equal(test_obj.ctx.current, &filling_states[IDLE],
		      "State did not transition back to IDLE"
		      "Current state: %p",
		      test_obj.ctx.current);
}
