#include "data_models.h"
#include "packets.h"
#include "services/state_machine/filling_sm_config.h"
#include "services/state_machine/main_sm.h"

#include "zephyr/kernel.h"
#include "zephyr/zbus/zbus.h"
#include "zephyr/logging/log.h"

static struct sm_object sm_obj;
bool smf_run_scheduled = false;

LOG_MODULE_DECLARE(state_machine_service);

#define SM_WORK_Q_PRIO 5                      // TODO: make KConfig
K_THREAD_STACK_DEFINE(sm_work_q_stack, 1024); // TODO: Make KConfig
static struct k_work_q sm_work_q;

static void command_work_handler(struct k_work *work);
static void weight_work_handler(struct k_work *work);
static void thermo_work_handler(struct k_work *work);
static void pressure_work_handler(struct k_work *work);
static void state_machine_work_handler(struct k_work *work);

static K_WORK_DEFINE(command_work, command_work_handler);
static K_WORK_DEFINE(weight_work, weight_work_handler);
static K_WORK_DEFINE(thermo_work, thermo_work_handler);
static K_WORK_DEFINE(pressure_work, pressure_work_handler);
static K_WORK_DEFINE(state_machine_work, state_machine_work_handler);

// Subscribed channels
ZBUS_CHAN_DECLARE(chan_packets);
ZBUS_CHAN_DECLARE(chan_weight_sensors, chan_pressure_sensors, chan_thermo_sensors);

// Published channels
ZBUS_CHAN_DECLARE(chan_rocket_state);

static void rocket_state_listener_cb(const struct zbus_channel *chan);

ZBUS_LISTENER_DEFINE(rocket_state_listener, rocket_state_listener_cb);
ZBUS_CHAN_ADD_OBS(chan_packets, rocket_state_listener, 5);          // FIXME: Make KConfig
ZBUS_CHAN_ADD_OBS(chan_weight_sensors, rocket_state_listener, 5);   // FIXME: Make KConfig
ZBUS_CHAN_ADD_OBS(chan_thermo_sensors, rocket_state_listener, 5);   // FIXME: Make KConfig
ZBUS_CHAN_ADD_OBS(chan_pressure_sensors, rocket_state_listener, 5); // FIXME: Make KConfig

#define ZBUS_RET_CHECK(ret, work)                                                             \
    switch (ret)                                                                              \
    {                                                                                         \
    case -EFAULT:                                                                             \
        LOG_ERR("Error reading packet channel");                                              \
        return;                                                                               \
    case -EAGAIN:                                                                             \
        LOG_WRN("No new packet to read");                                                     \
        return;                                                                               \
    case -EBUSY:                                                                              \
        /* try again*/                                                                        \
        LOG_WRN("Packet channel busy, retrying");                                             \
        k_work_submit_to_queue(&sm_work_q, work);                                             \
        return;                                                                               \
    case 0:                                                                                   \
        break;                                                                                \
    default:                                                                                  \
        LOG_ERR("Unexpected error reading packet channel: %d", ret);                          \
        return;                                                                               \
    }

#define CHECK_PARAMS(params, cmd_name)                                                        \
    if (!params)                                                                              \
    {                                                                                         \
        LOG_ERR("Received " #cmd_name " with no params");                                     \
        return;                                                                               \
    }

#define ASSIGN_NON_ZERO(dest, src)                                                            \
    if (src != 0)                                                                             \
    {                                                                                         \
        dest = src;                                                                           \
    }

#define SCHEDULE_SM_RUN()                                                                     \
    if (!smf_run_scheduled)                                                                   \
    {                                                                                         \
        smf_run_scheduled = true;                                                             \
        k_work_submit_to_queue(&sm_work_q, &state_machine_work);                              \
    }

static void rocket_state_listener_cb(const struct zbus_channel *chan)
{
    if (chan == &chan_packets)
    {
        k_work_submit_to_queue(&sm_work_q, &command_work);
        return;
    }

    if (chan == &chan_weight_sensors)
    {
        k_work_submit_to_queue(&sm_work_q, &weight_work);
        return;
    }

    if (chan == &chan_thermo_sensors)
    {
        k_work_submit_to_queue(&sm_work_q, &thermo_work);
        return;
    }

    if (chan == &chan_pressure_sensors)
    {
        k_work_submit_to_queue(&sm_work_q, &pressure_work);
        return;
    }
}

static void command_work_handler(struct k_work *work)
{
    struct generic_packet_s generic_packet;
    int ret = zbus_chan_read(&chan_packets, &generic_packet, K_NO_WAIT);
    ZBUS_RET_CHECK(ret, &command_work);

    command_t cmd = (command_t)generic_packet.header.command_id;
    if (cmd != CMD_FILL_EXEC)
    {
        return;
    }

    struct filling_sm_config *fill_cfg = &sm_obj.config->filling_sm_config;

    struct cmd_fill_exec_s *fill_exec = (struct cmd_fill_exec_s *)&generic_packet;
    fill_command_t fill_cmd = (fill_command_t)fill_exec->payload.program_id;

    switch (fill_cmd)
    {
    case CMD_FILL_N2:
    {
        struct fill_N2_params_s *params = (struct fill_N2_params_s *)fill_exec->payload.params;
        CHECK_PARAMS(params, CMD_FILL_N2);

        uint16_t target_n2 = params->target_N2_deci_bar;
        uint16_t trigger_n2 = params->trigger_N2_deci_bar;

        ASSIGN_NON_ZERO(fill_cfg->fill_n2.target_n2_tank_pressure, target_n2);
        ASSIGN_NON_ZERO(fill_cfg->fill_n2.trigger_n2_tank_pressure, trigger_n2);

        break;
    }

    case CMD_FILL_PRE_PRESS:
    {
        struct fill_press_params_s *params =
            (struct fill_press_params_s *)fill_exec->payload.params;
        CHECK_PARAMS(params, CMD_FILL_PRE_PRESS);

        uint16_t target_pre = params->target_tank_deci_bar;
        uint16_t trigger_pre = params->trigger_tank_deci_bar;

        ASSIGN_NON_ZERO(fill_cfg->pre_press.target_n2o_tank_pressure, target_pre);
        ASSIGN_NON_ZERO(fill_cfg->pre_press.trigger_n2o_tank_pressure, trigger_pre);

        break;
    }

    case CMD_FILL_N2O:
    {
        struct fill_N2O_params_s *params =
            (struct fill_N2O_params_s *)fill_exec->payload.params;
        CHECK_PARAMS(params, CMD_FILL_N2O);

        uint16_t target_wt = params->target_weight_grams;
        int16_t trigger_temp = params->trigger_temp_deci_c;

        ASSIGN_NON_ZERO(fill_cfg->fill_n2o.target_n2o_tank_weight, target_wt);
        ASSIGN_NON_ZERO(fill_cfg->fill_n2o.trigger_n2o_tank_temperature, trigger_temp);

        break;
    }

    case CMD_FILL_POST_PRESS:
    {
        struct fill_press_params_s *params =
            (struct fill_press_params_s *)fill_exec->payload.params;

        CHECK_PARAMS(params, CMD_FILL_POST_PRESS);

        uint16_t target_post = params->target_tank_deci_bar;
        uint16_t trigger_post = params->trigger_tank_deci_bar;

        ASSIGN_NON_ZERO(fill_cfg->post_press.target_n2o_tank_pressure, target_post);
        ASSIGN_NON_ZERO(fill_cfg->post_press.trigger_n2o_tank_pressure, trigger_post);

        break;
    }

    default:
        LOG_ERR("Received fill exec with unknown fill command: %d", fill_cmd);
        return;
    }

    sm_obj.command = cmd;
    sm_obj.fill_command = fill_cmd;

    SCHEDULE_SM_RUN();
}

static void weight_work_handler(struct k_work *work)
{
    loadcell_weights_t weights;
    int ret = zbus_chan_read(&chan_weight_sensors, &weights, K_NO_WAIT);
    ZBUS_RET_CHECK(ret, &command_work);

    sm_obj.data.loadcells = weights;
    SCHEDULE_SM_RUN();
}

static void thermo_work_handler(struct k_work *work)
{
    thermocouples_t thermos;
    int ret = zbus_chan_read(&chan_thermo_sensors, &thermos, K_NO_WAIT);
    ZBUS_RET_CHECK(ret, &command_work);

    sm_obj.data.thermocouples = thermos;
    SCHEDULE_SM_RUN();
}

static void pressure_work_handler(struct k_work *work)
{
    pressures_t pressures;
    int ret = zbus_chan_read(&chan_pressure_sensors, &pressures, K_NO_WAIT);
    ZBUS_RET_CHECK(ret, &command_work);

    sm_obj.data.pressures = pressures;
    SCHEDULE_SM_RUN();
}

static void state_machine_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);

    smf_run_scheduled = false;
    smf_run_state(SMF_CTX(&sm_obj));

    // Clear commands after processing
    sm_obj.command = 0;
    sm_obj.fill_command = 0;

    int ret = zbus_chan_pub(&chan_rocket_state, &sm_obj.state_data, K_NO_WAIT);
    if (ret != 0)
    {
        LOG_ERR("Failed to publish rocket state: %d", ret);
    }
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
    // LOG_WRN_ONCE("NOT IMPLEMENTED");
    // return;
    k_work_queue_start(&sm_work_q, sm_work_q_stack, K_THREAD_STACK_SIZEOF(sm_work_q_stack),
                       SM_WORK_Q_PRIO, NULL);
}
