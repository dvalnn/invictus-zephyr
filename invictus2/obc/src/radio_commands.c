#include "radio_commands.h"

#include <string.h>

#define CHECK_ARGS(cmd, buf, buf_size)                                                        \
    do {                                                                                      \
        if (!cmd || !buf) {                                                                   \
            return PACK_ERROR_INVALID_ARG;                                                    \
        }                                                                                     \
        if (buf_size < RADIO_CMD_SIZE) {                                                      \
            return PACK_ERROR_BUFFER_TOO_SMALL;                                               \
        }                                                                                     \
    } while (0)

#define CHECK_CMD(cmd)                                                                        \
    do {                                                                                      \
        if (cmd->header.packet_version != SUPPORTED_PACKET_VERSION) {                         \
            return PACK_ERROR_UNSUPPORTED_VERSION;                                            \
        }                                                                                     \
        if (cmd->header.command_id <= RCMD_NONE || cmd->header.command_id >= RCMD_MAX) {      \
            return PACK_ERROR_INVALID_CMD_ID;                                                 \
        }                                                                                     \
    } while (0)

enum pack_error_e radio_cmd_pack(const struct radio_generic_cmd_s *const cmd,
                                 uint8_t *const out_buf, const size_t out_buf_size)
{
    CHECK_ARGS(cmd, out_buf, out_buf_size);
    CHECK_CMD(cmd);
    memcpy(out_buf, cmd, RADIO_CMD_SIZE);
    return PACK_ERROR_NONE;
}

enum pack_error_e radio_cmd_unpack(const uint8_t *const in_buf, const size_t in_buf_size,
                                   struct radio_generic_cmd_s *const cmd)
{
    CHECK_ARGS(cmd, in_buf, in_buf_size);
    memcpy(cmd, in_buf, RADIO_CMD_SIZE);
    CHECK_CMD(cmd);
    return PACK_ERROR_NONE;
}
