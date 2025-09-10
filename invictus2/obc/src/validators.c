/* NEED TO UPDATE TO NEW STATE MACHINE

#include "validators.h"

#include "data_models.h"
#include "commands.h"

bool validate_fill_exec_cmd(const struct cmd_fill_exec_s *const cmd)
{
    // TODO: Validate the params for sane values
    switch ((enum fill_program_e)cmd->payload.program_id) {
    case FILL_PROGRAM_N2: {
        return true;

        // REVIEW: test if this kind of cast works as expected. Same for the ones below.
        // const struct fill_N2_params_s *const params = 
        //     (const struct fill_N2_params_s *const)cmd->payload.params; 
    }

    case FILL_PROGRAM_N2O: {
        return true;

        // const struct fill_N2O_params_s *const params = 
        //     (const struct fill_N2O_params_s *const)cmd->payload.params; 
    }

    case FILL_PROGRAM_PRE_PRESS:
    case FILL_PROGRAM_POST_PRESS: {
        return true;

        // const struct fill_press_params_s *const params =
        // (const struct fill_press_params_s *const) cmd->payload.params; 
    }

    case _FILL_PROGRAM_NONE:
    case _FILL_PROGRAM_MAX:
    default:
        return false;
    }
}

bool validate_manual_exec_cmd(const struct cmd_manual_exec_s *const cmd)
{
    switch ((enum manual_cmd_e)cmd->payload.manual_cmd_id) {
    case MANUAL_CMD_SD_LOG_START:
    case MANUAL_CMD_SD_LOG_STOP:
    case MANUAL_CMD_SD_STATUS:
        return true; // No parameters to validate

    case MANUAL_CMD_VALVE_STATE:
    case MANUAL_CMD_VALVE_MS:
    case MANUAL_CMD_LOADCELL_TARE:
    case MANUAL_CMD_TANK_TARE:
        // TODO: Validate the params for sane values
        return true;

    case _MANUAL_CMD_NONE:
    case _MANUAL_CMD_MAX:
    default:
        return false;
    }
}

bool radio_cmd_validator(const void *msg, size_t msg_size)
{
    const struct generic_cmd_s *const cmd = msg;

    // FIXME: Compare sender ID to Ground Station ID and receiver ID to Rocket ID
    bool is_supported_and_from_gs_to_obc =
        (msg != NULL) && (msg_size == sizeof(*cmd)) &&
        (cmd->header.packet_version == SUPPORTED_PACKET_VERSION) &&
        (cmd->header.sender_id != 0) && (cmd->header.target_id != 0);

    if (!is_supported_and_from_gs_to_obc) {
        return false;
    }

    // Switch on the command ID to validate command-specific fields
    switch ((enum command_e)cmd->header.command_id) {
    case CMD_STATUS_REQ:
    case CMD_ABORT:
    case CMD_READY:
    case CMD_ARM:
    case CMD_FIRE:
    case CMD_LAUNCH_OVERRIDE:
    case CMD_FILL_STOP:
    case CMD_FILL_RESUME:
    case CMD_MANUAL_TOGGLE:
        return true; // No payload data, just reserved bytes

    case CMD_FILL_EXEC:
        return validate_fill_exec_cmd((const struct cmd_fill_exec_s *const)cmd);

    case CMD_MANUAL_EXEC:
        return validate_manual_exec_cmd((const struct cmd_manual_exec_s *const)cmd);

    case CMD_STATUS_REP: // Should not be received
    case CMD_ACK:        // Should not be received
    default:
        return false;
    }

    return is_supported_and_from_gs_to_obc;
}

bool rocket_state_validator(const void *msg, size_t msg_size)
{
    const struct rocket_state_s *const state = msg;

    if ((msg_size != sizeof(*state)) || (state == NULL)) {
        return false;
    }

    switch ((enum rocket_state_e)state->major) {

    case ROCKET_STATE_IDLE:
    case ROCKET_STATE_ABORT:
    case ROCKET_STATE_READY:
    case ROCKET_STATE_ARMED:
        return state->minor == 0;

    case ROCKET_STATE_FUEL:
        return (state->minor > _RS_FILL_NONE) && (state->minor < _RS_FILL_MAX);

    case ROCKET_STATE_FLIGHT:
        return (state->minor > _FLIGHT_NONE) && (state->minor < _RS_FLIGHT_MAX);

    default:
        return false;
    }
}

*/