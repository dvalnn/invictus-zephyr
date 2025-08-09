#include "radio_commands.h"

#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(radio_commands, LOG_LEVEL_DBG);

int radio_cmd_pack(const struct radio_generic_cmd_s *const cmd, uint8_t *const out_buf,
                   const size_t out_buf_size)
{
    if (!cmd || !out_buf || out_buf_size < RADIO_CMD_SIZE) {
        return -1; // Error: Invalid parameters
    }

    memcpy(out_buf, cmd, RADIO_CMD_SIZE);
    return 0;
}

int radio_cmd_unpack(const uint8_t *const in_buf, const size_t in_buf_size,
                     struct radio_generic_cmd_s *const cmd)
{
    if (!in_buf || !cmd || in_buf_size < RADIO_CMD_SIZE) {
        LOG_ERR("Invalid parameters for unpack");
        return -1; // Error: Invalid parameters
    }

    // unpack the entire command at once
    memcpy(cmd, in_buf, RADIO_CMD_SIZE);

    // validate the command header
    if (cmd->header.packet_version != SUPPORTED_PACKET_VERSION) {
        LOG_ERR("Invalid packet version: %d", cmd->header.packet_version);
        return -1; // Error: Invalid packet version
    }

    if (cmd->header.command_id <= RCMD_NONE || cmd->header.command_id >= RCMD_MAX) {
        LOG_ERR("Invalid command ID: %d", cmd->header.command_id);
        return -1; // Error: Invalid command ID
    }

    return 0;
}
