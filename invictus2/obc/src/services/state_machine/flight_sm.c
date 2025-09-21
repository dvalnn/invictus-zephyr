#include "services/state_machine/flight_sm.h"
#include "services/state_machine/main_sm.h"

#include <zephyr/toolchain.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/kernel.h>

LOG_MODULE_DECLARE(state_machine_service);

#define SET_FLIGHT_STATE(s, st) SET_STATE(s, FLIGHT_SST(st))

// Helper functions

#define BOOST_TIME_MS 4000 // FIXME: Make KConfig
bool boost_timer_expired = false;
bool boost_timer_cancelled = false;

void boost_timer_expiry_fn(struct k_timer *timer_id);
void boost_timer_stop_fn(struct k_timer *timer_id);

K_TIMER_DEFINE(boost_timer, boost_timer_expiry_fn, boost_timer_stop_fn);

void boost_timer_start(void)
{
    k_timer_start(&boost_timer, K_MSEC(BOOST_TIME_MS), K_NO_WAIT);
    boost_timer_expired = false;
    boost_timer_cancelled = false;
}

void boost_timer_expiry_fn(struct k_timer *timer_id)
{
    ARG_UNUSED(timer_id);
    boost_timer_expired = true;
    boost_timer_cancelled = false;
}

void boost_timer_stop_fn(struct k_timer *timer_id)
{
    ARG_UNUSED(timer_id);
    if (!boost_timer_expired)
    {
        boost_timer_cancelled = true;
    }
}

/* =============================================================================== */
/* root/flight/ignition: Waiting for temperature to reach min value for combustion */
/* =============================================================================== */
void ignition_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.flight_state = IGNITION;
}

void ignition_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    if (s->data.thermocouples.chamber_thermo >
        s->config->flight_sm_config.min_chamber_launch_temp)
    {
        SET_FLIGHT_STATE(s, BOOST);
    }
}

void ignition_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* root/flight/boost: Rocket is boosting upwards */
/* ======================================================================== */
void boost_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.flight_state = BOOST;
    boost_timer_start();
}

void boost_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;

    if (boost_timer_expired || boost_timer_cancelled)
    {

        int16_t kalman_vs = s->data.kalman.vertical_speed;
        int16_t config_vs = s->config->flight_sm_config.boost_vertical_speed;
        if (kalman_vs >= config_vs)
        {
            LOG_WRN("Boost timer expired. Vertical speed is higher than expected: %d >= %d ",
                    kalman_vs, config_vs);
        }

        if (boost_timer_cancelled)
        {
            LOG_WRN("Boost timer cancelled, transitioning to COAST");
        }

        SET_FLIGHT_STATE(s, COAST);
    }
}

void boost_exit(void *o)
{
    k_timer_stop(&boost_timer);
}

/* ======================================================================== */
/* root/flight/coast: Rocket is coasting upwards */
/* ======================================================================== */
void coast_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.flight_state = COAST;
}

void coast_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    // Transition to APOGEE when vertical speed drops below threshold.
    if (s->data.kalman.vertical_speed < s->config->flight_sm_config.coast_vertical_speed)
    {
        SET_FLIGHT_STATE(s, APOGEE);
    }
}

void coast_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* root/flight/apogee: Rocket has reached apogee */
/* ======================================================================== */
void apogee_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.flight_state = APOGEE;
}

void apogee_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    // Transition to DROGUE_CHUTE when ematch is fired.
    if (s->data.actuators.ematch_drogue == 1)
    {
        SET_FLIGHT_STATE(s, DROGUE_CHUTE);
    }
}

void apogee_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* root/flight/drogue_chute: Deploying drogue chute */
/* ======================================================================== */
void drogue_chute_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.flight_state = DROGUE_CHUTE;
}

void drogue_chute_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    // Transition to MAIN_CHUTE when altitude drops below threshold.
    if (s->data.kalman.altitude < s->config->flight_sm_config.main_chute_deploy_altitude)
    {
        SET_FLIGHT_STATE(s, MAIN_CHUTE);
    }
}

void drogue_chute_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* root/flight/main_chute: Deploying main chute */
/* ======================================================================== */
void main_chute_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.flight_state = MAIN_CHUTE;
}

void main_chute_run(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    // Transition to TOUCHDOWN when altitude drops below threshold.
    if (s->data.kalman.altitude < s->config->flight_sm_config.touchdown_altitude)
    {
        SET_FLIGHT_STATE(s, TOUCHDOWN);
    }
}

void main_chute_exit(void *o)
{
    ARG_UNUSED(o);
}

/* ======================================================================== */
/* root/flight/touchdown: Rocket has touched down */
/* ======================================================================== */
void touchdown_entry(void *o)
{
    struct sm_object *s = (struct sm_object *)o;
    s->state_data.flight_state = TOUCHDOWN;
}

void touchdown_run(void *o)
{
    ARG_UNUSED(o);
}

void touchdown_exit(void *o)
{
    ARG_UNUSED(o);
}
