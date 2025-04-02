#include <zephyr/smf.h>

/* Forward declaration of state table */
static const struct smf_state filling_states[];

/* List of possible states */
enum filling_state {
    // TOP Level
    ABORT,
    IDLE,
    MANUAL_OP, // NOTE: Acept manual override commands from the ground station

    // SAFE PAUSE
    SAFE_PAUSE, // parent state
    SAFE_PAUSE_IDLE,
    SAFE_PAUSE_VENT,

    // FILLING_COPV (filling N)
    FILLING_COPV, // parent state
    FILLING_COPV_IDLE,
    FILLING_COPV_FILL,

    // PRE_PRESSURIZING
    PRE_PRESSURIZING, // parent state
    PRE_PRESSURIZING_IDLE,
    PRE_PRESSURIZING_VENT,
    PRE_PRESSURIZING_FILL_N,

    // FILLING_N20
    FILLING_N20, // parent state
    FILLING_N20_IDLE,
    FILLING_N20_FILL,
    FILLING_N20_VENT,

    // POST_PRESSURIZING
    POST_PRESSURIZING, // parent state
    POST_PRESSURIZING_IDLE,
    POST_PRESSURIZING_VENT,
    POST_PRESSURIZING_FILL_N,
};

/* User defined object */
struct s_object {
    /* This must be first */
    struct smf_ctx ctx;

    /* Other state specific data add here */
} s_obj;

/* State Callbacks */

static void abort_entry(void *o) {
    struct s_object *s = (struct s_object *)o;
    (void)s; // unused

    // Do something
    // ...
}

static void abort_run(void *o) {
    struct s_object *s = (struct s_object *)o;
    (void)s; // unused

    // Do something
    // ...
}

static void abort_exit(void *o) {
    struct s_object *s = (struct s_object *)o;
    (void)s; // unused

    // Do something
    // ...
}

/* Populate State table */
static const struct smf_state filling_states[] = {

    // SMF_CREATE_STATE(s_entry_cb, s_run_cb, s_exit_cb, s_parent, s_initial_o),

    [ABORT] = SMF_CREATE_STATE(abort_entry, abort_run, abort_exit, NULL, NULL),
    [IDLE] = SMF_CREATE_STATE(NULL, NULL, NULL, NULL, NULL),
    [MANUAL_OP] = SMF_CREATE_STATE(NULL, NULL, NULL, NULL, NULL),

    [SAFE_PAUSE] = SMF_CREATE_STATE(NULL, NULL, NULL, NULL, NULL),
    [SAFE_PAUSE_IDLE] = SMF_CREATE_STATE(NULL, NULL, NULL, &filling_states[SAFE_PAUSE], NULL),
    [SAFE_PAUSE_VENT] = SMF_CREATE_STATE(NULL, NULL, NULL, &filling_states[SAFE_PAUSE], NULL),

    [FILLING_COPV] = SMF_CREATE_STATE(NULL, NULL, NULL, NULL, NULL),
    [FILLING_COPV_IDLE] = SMF_CREATE_STATE(NULL, NULL, NULL, &filling_states[FILLING_COPV], NULL),
    [FILLING_COPV_FILL] = SMF_CREATE_STATE(NULL, NULL, NULL, &filling_states[FILLING_COPV], NULL),

    [PRE_PRESSURIZING] = SMF_CREATE_STATE(NULL, NULL, NULL, NULL, NULL),
    [PRE_PRESSURIZING_IDLE] = SMF_CREATE_STATE( NULL, NULL, NULL, &filling_states[PRE_PRESSURIZING], NULL),
    [PRE_PRESSURIZING_VENT] = SMF_CREATE_STATE( NULL, NULL, NULL, &filling_states[PRE_PRESSURIZING], NULL),
    [PRE_PRESSURIZING_FILL_N] = SMF_CREATE_STATE( NULL, NULL, NULL, &filling_states[PRE_PRESSURIZING], NULL),

    [FILLING_N20] = SMF_CREATE_STATE(NULL, NULL, NULL, NULL, NULL),
    [FILLING_N20_IDLE] = SMF_CREATE_STATE(NULL, NULL, NULL, &filling_states[FILLING_N20], NULL),
    [FILLING_N20_FILL] = SMF_CREATE_STATE(NULL, NULL, NULL, &filling_states[FILLING_N20], NULL),
    [FILLING_N20_VENT] = SMF_CREATE_STATE(NULL, NULL, NULL, &filling_states[FILLING_N20], NULL),

    [POST_PRESSURIZING] = SMF_CREATE_STATE(NULL, NULL, NULL, NULL, NULL),
    [POST_PRESSURIZING_IDLE] = SMF_CREATE_STATE( NULL, NULL, NULL, &filling_states[POST_PRESSURIZING], NULL),
    [POST_PRESSURIZING_VENT] = SMF_CREATE_STATE( NULL, NULL, NULL, &filling_states[POST_PRESSURIZING], NULL),
    [POST_PRESSURIZING_FILL_N] = SMF_CREATE_STATE( NULL, NULL, NULL, &filling_states[POST_PRESSURIZING], NULL),
};
