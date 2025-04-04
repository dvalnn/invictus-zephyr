#ifndef _FILLING_SM_H_
#define _FILLING_SM_H_

#include <zephyr/smf.h>

#define CMD_OTHER_START  0x00010000
#define CMD_IDLE_START   0x00000100
#define CMD_GLOBAL_START 0x00000001

#define CMD_OTHER_MASK  0x00FF0000
#define CMD_IDLE_MASK   0x0000FF00
#define CMD_GLOBAL_MASK 0x000000FF

#define CMD_OTHER(x)  ((x) & CMD_OTHER_MASK)
#define CMD_IDLE(x)   ((x) & CMD_IDLE_MASK)
#define CMD_GLOBAL(x) ((x) & CMD_GLOBAL_MASK)

enum cmd_global {
	CMD_STOP = CMD_GLOBAL_START, // ANY -> IDLE
	CMD_ABORT,                   // ANY -> ABORT
	CMD_PAUSE,                   // ANY -> SAFE_PAUSE_IDLE
};

enum cmd_idle {
	CMD_FILL_COPV = CMD_IDLE_START, // IDLE -> FILLING_COPV_IDLE
	CMD_PRE_PRESSURIZE,             // IDLE -> PRE_PRESSURIZING_IDLE
	CMD_FILL_N20,                   // IDLE -> FILLING_N20_IDLE
	CMD_POST_PRESSURIZE,            // IDLE -> POST_PRESSURIZING_IDLE
};

enum cmd_other {
	CMD_READY = CMD_OTHER_START, // ABORT -> IDLE
	CMD_RESUME,                  // SAFE_PAUSE -> IDLE
};

typedef uint32_t cmd_t;

/* List of possible states */
enum filling_state {
	IDLE,
	ABORT,
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
struct filling_sm_object {
	/* This must be first */
	struct smf_ctx ctx;

	cmd_t command;
	int modbus_client_iface;

	union filling_data {
		struct f_data_fields {
			uint16_t n_pressure;
			uint16_t n2o_pressure;
			uint16_t n2o_weight;
			uint16_t temperature;
		} fields;
		uint16_t raw[4];
	} data;

	struct safe_pause_config {
		uint16_t target_np;
		uint16_t trigger_np;
	} s_p_config;

	// State Machine Configuration
	struct filling_copv_config {
		uint16_t target_np;
	} f_copv_config;

	struct pre_pressurizing_config {
		uint16_t target_n2op;
		uint16_t trigger_n2op;
	} pre_p_config;

	struct filling_n20_config {
		uint16_t target_n2op;
		uint16_t target_weight;
		uint16_t trigger_n2op;
		uint16_t trigger_temp;
	} f_n20_config;

	struct post_pressurizing_config {
		uint16_t target_n2op;
		uint16_t trigger_n2op;
	} post_p_config;
};

#ifdef UNIT_TEST
extern const struct smf_state filling_states[];
#endif

void filling_sm_init(struct filling_sm_object *initial_s_obj);

#endif // _FILLING_SM_H_
