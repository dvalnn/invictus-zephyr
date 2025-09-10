#include "services/state_machine/flight_sm.h"

/* ======================================================================== */
/* flight/ignition: Waiting for temperature to reach min value for combustion */
/* ======================================================================== */
void ignition_entry(void *o) {
    ARG_UNUSED(o);
}

void ignition_run(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    if (s->data.thermocouples.chamber_thermo > s->config->flight_sm_config.min_chamber_launch_temp) {
        smf_set_state(SMF_CTX(s), &states[BOOST]);
    }
}

void ignition_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* flight/boost: Rocket is boosting upwards */
/* ======================================================================== */
void boost_entry(void *o) {
    ARG_UNUSED(o);
}

void boost_run(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    // FIXME: Transition to COAST should be after X seconds of boost.
    if (s->data.kalman.vertical_speed < s->config->flight_sm_config.boost_vertical_speed) {
        smf_set_state(SMF_CTX(s), &states[COAST]);
    }
}

void boost_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* flight/coast: Rocket is coasting upwards */
/* ======================================================================== */
void coast_entry(void *o) {
    ARG_UNUSED(o);
}

void coast_run(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    // Transition to APOGEE when vertical speed drops below threshold.
    if (s->data.kalman.vertical_speed < s->config->flight_sm_config.coast_vertical_speed) {
        smf_set_state(SMF_CTX(s), &states[APOGEE]);
    }
}

void coast_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* flight/apogee: Rocket has reached apogee */
/* ======================================================================== */
void apogee_entry(void *o) {
    ARG_UNUSED(o);
}

void apogee_run(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    // Transition to DROGUE_CHUTE when ematch is fired.
    if (s->data.actuators.ematch_drogue == 1) {
        smf_set_state(SMF_CTX(s), &states[DROGUE_CHUTE]);
    }
}

void apogee_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* flight/drogue_chute: Deploying drogue chute */
/* ======================================================================== */
void drogue_chute_entry(void *o) {
    ARG_UNUSED(o);
}

void drogue_chute_run(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    // Transition to MAIN_CHUTE when altitude drops below threshold.
    if (s->data.kalman.altitude < s->config->flight_sm_config.main_chute_deploy_altitude) {
        smf_set_state(SMF_CTX(s), &states[MAIN_CHUTE]);
    }
}

void drogue_chute_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* flight/main_chute: Deploying main chute */
/* ======================================================================== */
void main_chute_entry(void *o) {
    ARG_UNUSED(o);
}

void main_chute_run(void *o) {
    struct sm_object *s = (struct sm_object *)o;
    // Transition to TOUCHDOWN when altitude drops below threshold.
    if (s->data.kalman.altitude < s->config->flight_sm_config.touchdown_altitude) {
        smf_set_state(SMF_CTX(s), &states[TOUCHDOWN]);
    }
}

void main_chute_exit(void *o) {
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* flight/touchdown: Rocket has touched down */
/* ======================================================================== */
void touchdown_entry(void *o) {
    ARG_UNUSED(o);
}

void touchdown_run(void *o) {
    ARG_UNUSED(o);
}

void touchdown_exit(void *o) {
    ARG_UNUSED(o);
}

