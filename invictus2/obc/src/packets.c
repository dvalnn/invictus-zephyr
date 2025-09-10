#include "packets.h"

#include <string.h>

#define CHECK_ARGS(packet, buf, buf_size)                                                        \
    do {                                                                                      \
        if (!packet || !buf) {                                                                   \
            return PACK_ERROR_INVALID_ARG;                                                    \
        }                                                                                     \
        if (buf_size < PACKET_SIZE) {                                                      \
            return PACK_ERROR_BUFFER_TOO_SMALL;                                               \
        }                                                                                     \
    } while (0)

#define CHECK_PACKET(packet)                                                                        \
    do {                                                                                      \
        if (packet->header.packet_version != SUPPORTED_PACKET_VERSION) {                         \
            return PACK_ERROR_UNSUPPORTED_VERSION;                                            \
        }                                                                                     \
    if (packet->header.command_id <= _CMD_NONE || packet->header.command_id >= _CMD_MAX) {    \
            return PACK_ERROR_INVALID_CMD_ID;                                                 \
        }                                                                                     \
    } while (0)

enum pack_error_e packet_pack(const struct generic_packet_s *const packet,
                                 uint8_t *const out_buf, const size_t out_buf_size)
{
    CHECK_ARGS(packet, out_buf, out_buf_size);
    CHECK_PACKET(packet);
    memcpy(out_buf, packet, PACKET_SIZE);
    return PACK_ERROR_NONE;
}

enum pack_error_e packet_unpack(const uint8_t *const in_buf, const size_t in_buf_size,
                                   struct generic_packet_s *const packet)
{
    CHECK_ARGS(packet, in_buf, in_buf_size);
    memcpy(packet, in_buf, PACKET_SIZE);
    CHECK_PACKET(packet);
    return PACK_ERROR_NONE;
}
