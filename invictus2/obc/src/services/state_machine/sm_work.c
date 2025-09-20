#include "services/state_machine/main_sm.h"

#include "zephyr/kernel.h"
#include "zephyr/zbus/zbus.h"
#include "zephyr/logging/log.h"

static struct sm_object sm_obj;

LOG_MODULE_DECLARE(state_machine_service);

#define SM_WORK_Q_PRIO 5               // TODO: make KConfig
K_THREAD_STACK_DEFINE(sm_work_q_stack, 1024); // TODO: Make KConfig
static struct k_work_q sm_work_q;

#define SM_THREAD_PRIO 5               // TODO: make KConfig
K_THREAD_STACK_DEFINE(sm_stack, 1024); // TODO: make KConfig
static struct k_thread sm_thread;

static void command_work_handler(struct k_work *work);
static void weight_work_handler(struct k_work *work);
static void thermo_work_handler(struct k_work *work);
static void pressure_work_handler(struct k_work *work);

static K_WORK_DEFINE(command_work, command_work_handler);
static K_WORK_DEFINE(weight_work, weight_work_handler);
static K_WORK_DEFINE(thermo_work, thermo_work_handler);
static K_WORK_DEFINE(pressure_work, pressure_work_handler);

// Subscribed channels
ZBUS_CHAN_DECLARE(chan_packets);
ZBUS_CHAN_DECLARE(chan_weight_sensors, chan_pressure_sensors, chan_thermo_sensors);

// Published channels
ZBUS_CHAN_DECLARE(chan_rocket_state);

static void rocket_state_listener_cb(const struct zbus_channel *chan);

ZBUS_LISTENER_DEFINE(rocket_state_listener, rocket_state_listener_cb);
ZBUS_CHAN_ADD_OBS(chan_packets, rocket_state_listener, 5);       // FIXME: Make KConfig
ZBUS_CHAN_ADD_OBS(chan_weight_sensors, rocket_state_listener, 5);   // FIXME: Make KConfig
ZBUS_CHAN_ADD_OBS(chan_thermo_sensors, rocket_state_listener, 5);   // FIXME: Make KConfig
ZBUS_CHAN_ADD_OBS(chan_pressure_sensors, rocket_state_listener, 5); // FIXME: Make KConfig

static void rocket_state_listener_cb(const struct zbus_channel *chan)
{
    if (chan == &chan_packets) {
        k_work_submit_to_queue(&sm_work_q, &command_work);
        return;
    }

    if (chan == &chan_weight_sensors) {
        k_work_submit_to_queue(&sm_work_q, &weight_work);
        return;
    }

    if (chan == &chan_thermo_sensors) {
        k_work_submit_to_queue(&sm_work_q, &thermo_work);
        return;
    }

    if (chan == &chan_pressure_sensors) {
        k_work_submit_to_queue(&sm_work_q, &pressure_work);
        return;
    }
}

static void command_work_handler(struct k_work *work)
{
    LOG_INF("Entered cmd work handler");
    struct generic_packet_s generic_packet;
    int ret = zbus_chan_read(&chan_packets, &generic_packet, K_NO_WAIT);
    if (ret == 0) {
        sm_obj.command = generic_packet.header.command_id;
        LOG_INF("Received command: 0x%08x", sm_obj.command);
    }
    // Optionally: trigger state machine run or set a flag
}

static void weight_work_handler(struct k_work *work)
{
    k_oops(); // FIXME:
}

static void thermo_work_handler(struct k_work *work)
{
    k_oops(); // FIXME:
}

static void pressure_work_handler(struct k_work *work)
{
    k_oops(); // FIXME:
}

void sm_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    LOG_INF("SM thread starting");
    const uint32_t c_sleep_time_ms = 80; // FIXME: Make KConfig
    while (true) {
        // TODO: handle zbus stuff
        // read cmd bus

        //LOG_INF("SM doing stuff");

        smf_run_state(SMF_CTX(&sm_obj));

        // TODO sleep only difference
        k_sleep(K_MSEC(c_sleep_time_ms));
    }

    LOG_INF("SM thread exiting.");
}

bool state_machine_service_setup(void)
{
    // Perform any necessary setup for the state machine service here.
    // For example, initialize hardware, data models, or configuration.
    // Return true if setup is successful, false otherwise.
    sm_init(&sm_obj);
    k_work_queue_init(&sm_work_q);
    return true;
}

void state_machine_service_start(void)
{
    // Start the state machine service.
    // Typically, this would initialize the state machine object and set its initial state.
    //LOG_WRN_ONCE("NOT IMPLEMENTED");
    //return;
    k_work_queue_start(&sm_work_q, sm_work_q_stack,
                   K_THREAD_STACK_SIZEOF(sm_work_q_stack), SM_WORK_Q_PRIO,
                   NULL);

    const k_tid_t tid =
        k_thread_create(&sm_thread, sm_stack, K_THREAD_STACK_SIZEOF(sm_stack),
                        sm_thread_entry, NULL, NULL, NULL, SM_THREAD_PRIO, 0, K_NO_WAIT);

    k_thread_name_set(tid, "state_machine");

    
}


