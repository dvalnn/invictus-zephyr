#include <zephyr/ztest.h>
#include <zephyr/smf.h>

#include "filling_sm.h"

/* Global test object for the state machine */
static DEFAULT_FSM_OBJECT(test_obj, 0);

/* Reset the test object with default values */
static void filling_sm_suite_before(void *data)
{
    ARG_UNUSED(data);

    test_obj.command = 0; // clear commands

    // clear sensor data
    test_obj.data.main_tank_pressure = 0;
    test_obj.data.main_tank_weight = 0;
    test_obj.data.main_tank_temperature = 0;
    test_obj.data.pre_tank_pressure = 0;
}

ZTEST_SUITE(filling_sm_suite, NULL, NULL, filling_sm_suite_before, NULL, NULL);

/* Test: Verify that the initial state is IDLE */
ZTEST(filling_sm_suite, test_initial_state)
{

    /* Initialize the state machine */
    filling_sm_init(&test_obj);

    /* Check if the initial state is IDLE */
    zassert_equal(test_obj.ctx.current, &filling_states[IDLE], "Initial state is not IDLE");
}

/* Test: Transition from IDLE to FILLING_COPV_IDLE via CMD_FILL_COPV */
ZTEST(filling_sm_suite, test_idle_to_filling_copv)
{

    /* Initialize the state machine */
    filling_sm_init(&test_obj);

    /* Set command to CMD_FILL_COPV */
    test_obj.command = CMD_FILL_COPV;

    /* Run the state machine */
    smf_run_state(SMF_CTX(&test_obj));

    /* Check if the state has transitioned to FILLING_COPV_IDLE */
    zassert_equal(test_obj.ctx.current, &filling_states[FILLING_COPV_IDLE],
                  "State did not transition to FILLING_COPV_IDLE: "
                  "Current state: %p",
                  test_obj.ctx.current);

    test_obj.command = CMD_STOP;

    /* Run the state machine again */
    smf_run_state(SMF_CTX(&test_obj));

    /* Check if the state has transitioned back to IDLE */
    zassert_equal(test_obj.ctx.current, &filling_states[IDLE],
                  "State did not transition back to IDLE: "
                  "Current state: %p",
                  test_obj.ctx.current);
}
