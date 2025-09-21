#include "packets.h"
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(packet, LOG_LEVEL_INF);

#define CHECK_ARGS(packet, buf, buf_size)                                                     \
    do                                                                                        \
    {                                                                                         \
        if (!packet || !buf)                                                                  \
        {                                                                                     \
            LOG_INF("Pack err inv arg");                                                      \
            return PACK_ERROR_INVALID_ARG;                                                    \
        }                                                                                     \
        if (buf_size < PACKET_SIZE)                                                           \
        {                                                                                     \
            LOG_INF("Buffer too small");                                                      \
            return PACK_ERROR_BUFFER_TOO_SMALL;                                               \
        }                                                                                     \
    } while (0)

#define CHECK_PACKET(packet)                                                                  \
    do                                                                                        \
    {                                                                                         \
        if (packet->header.packet_version != SUPPORTED_PACKET_VERSION)                        \
        {                                                                                     \
            LOG_INF("Unsupported pack version: %d", packet->header.packet_version);           \
            return PACK_ERROR_UNSUPPORTED_VERSION;                                            \
        }                                                                                     \
        if (packet->header.command_id <= _CMD_NONE || packet->header.command_id >= _CMD_MAX)  \
        {                                                                                     \
            LOG_INF("Invalid cmd ID");                                                        \
            return PACK_ERROR_INVALID_CMD_ID;                                                 \
        }                                                                                     \
    } while (0)

enum pack_error_e packet_pack(const generic_packet_t *const packet, uint8_t *const out_buf,
                              const size_t out_buf_size)
{
    CHECK_ARGS(packet, out_buf, out_buf_size);
    CHECK_PACKET(packet);
    memcpy(out_buf, packet, PACKET_SIZE);
    return PACK_ERROR_NONE;
}

enum pack_error_e packet_unpack(const uint8_t *const in_buf, const size_t in_buf_size,
                                generic_packet_t *const packet)
{
    CHECK_ARGS(packet, in_buf, in_buf_size);
    memcpy(packet, in_buf, PACKET_SIZE);
    CHECK_PACKET(packet);
    return PACK_ERROR_NONE;
}

generic_packet_t create_cmd_packet(command_t cmd)
{
    packet_header_t header = {.packet_version = SUPPORTED_PACKET_VERSION,
                              .sender_id = OBC_PACKET_ID,
                              .target_id = GROUND_STATION_PACKET_ID,
                              .command_id = (uint8_t)cmd};
    generic_packet_t packet = {.header = header, .payload = {0}};
    return packet;
}
