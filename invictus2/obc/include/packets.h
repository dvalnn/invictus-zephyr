#ifndef PACKETS_H_
#define PACKETS_H_

#include "zephyr/toolchain.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "data_models.h"

// REVIEW: make this KConfig param?
#define SUPPORTED_PACKET_VERSION 1
#define OBC_PACKET_ID            2
#define GROUND_STATION_PACKET_ID 1

// -----------------------------------------------------------------------------
// Packet header
// -----------------------------------------------------------------------------

// Always 1 byte fields, packed for on-the-wire transmission.
typedef struct packet_header_s
{
    uint8_t packet_version;
    uint8_t sender_id;
    uint8_t target_id;
    uint8_t command_id; // use values from enum radio_command_e
} packet_header_t;

// -----------------------------------------------------------------------------
// Generic Packet Structure
// -----------------------------------------------------------------------------

#define PACKET_SIZE          128u
#define PACKET_HEADER_BYTES  ((uint8_t)sizeof(struct packet_header_s))
#define PACKET_PAYLOAD_BYTES (PACKET_SIZE - PACKET_HEADER_BYTES)

// NOTE: Generic packet structure, cast to a specific packet type to access values by name.
typedef struct generic_packet_s
{
    struct packet_header_s header;
    uint8_t payload[PACKET_PAYLOAD_BYTES];
} generic_packet_t;

// -----------------------------------------------------------------------------
// STATUS_REP payload layout
// -----------------------------------------------------------------------------

// NOTE: Alignment needs to be kept in mind.
//       It might be usable cross-platform even without compiler-specific attributes, as long
//       as the fields are defined in the same order and sizes are consistent across platforms.
//
//       For our use case, however, both the sender and receiver are the same platform
//       and compiler, so we can safely use memcpy to serialize and deserialize.
//
// For now status rep struct == system_data_t

// -----------------------------------------------------------------------------
// Fill Exec payloads
// -----------------------------------------------------------------------------

struct fill_N2_params_s
{
    uint16_t target_N2_deci_bar;
    uint16_t trigger_N2_deci_bar;
};

struct fill_press_params_s
{
    uint16_t target_tank_deci_bar;
    uint16_t trigger_tank_deci_bar; // optional: set to 0xFFFF if unused
};

struct fill_N2O_params_s
{
    uint16_t target_weight_grams;
    int16_t trigger_temp_deci_c;
};

// Fill exec generic envelope
struct fill_exec_s
{
    uint8_t program_id;                       // values from fill_command_t
    uint8_t params[PACKET_PAYLOAD_BYTES - 1]; // 1 byte for program_id, rest for params
};

#define FILL_N2_PARAM_BYTES       (sizeof(struct fill_N2_params_s))
#define FILL_PRESSURE_PARAM_BYTES (sizeof(struct fill_pressure_params_s))
#define FILL_N2O_EXTRA_BYTES      (sizeof(struct fill_N2O_extra_s))

// -----------------------------------------------------------------------------
// Manual Commands
// -----------------------------------------------------------------------------

enum manual_cmd_e
{
    _MANUAL_CMD_NONE = 0,

    MANUAL_CMD_SD_LOG_START,
    MANUAL_CMD_SD_LOG_STOP,
    MANUAL_CMD_SD_STATUS,
    MANUAL_CMD_VALVE_STATE,
    MANUAL_CMD_VALVE_MS,
    MANUAL_CMD_LOADCELL_TARE,
    MANUAL_CMD_TANK_TARE,

    _MANUAL_CMD_MAX
};

struct manual_valve_state_s
{
    uint8_t valve_id;
    uint8_t open; // boolean: 0 closed, 1 open
};

struct manual_valve_ms_s
{
    uint8_t valve_id;
    uint32_t duration_ms;
};

struct manual_tare_s
{
    uint8_t id; // loadcell or tank id
};

// same logic as struct fill_exec_s
struct manual_exec_s
{
    uint8_t manual_cmd_id;
    uint8_t params[PACKET_PAYLOAD_BYTES - 1];
};

// -----------------------------------------------------------------------------
// ACK payload
// -----------------------------------------------------------------------------
// ACK contains the command id being acknowledged and optional error code(s).
// Define a compact representation.

struct ack_s
{
    uint8_t ack_cmd_id;  // command_t that is acknowledged
    uint8_t status_code; // 0 = OK, non-zero = error
    uint8_t params[PACKET_PAYLOAD_BYTES - 2];
};

// -----------------------------------------------------------------------------
// Command IDs
// -----------------------------------------------------------------------------
typedef enum cmd_e
{
    _CMD_NONE = 0,

    // FROM GROUND STATION to OBC
    //
    // No payload data, just reserved bytes
    CMD_STATUS_REQ = 1,
    CMD_ABORT,
    CMD_READY,
    CMD_ARM,
    CMD_FIRE,
    CMD_LAUNCH_OVERRIDE,
    CMD_STOP,
    CMD_SAFE_PAUSE,
    CMD_RESUME,
    CMD_MANUAL_TOGGLE,

    // Commands with payload data
    CMD_FILL_EXEC,
    CMD_MANUAL_EXEC,

    // FROM OBC to GROUND STATION
    //
    CMD_STATUS_REP,
    CMD_ACK,

    _CMD_MAX
} command_t;

typedef enum fill_cmd_e
{
    CMD_FILL_NONE = 0,
    CMD_FILL_N2,
    CMD_FILL_PRE_PRESS,
    CMD_FILL_N2O,
    CMD_FILL_POST_PRESS,
} fill_command_t;

typedef struct
{
    struct packet_header_s header;
    command_t cmd;

} packet_t;

// -----------------------------------------------------------------------------
// Individual Command Definitions
// -----------------------------------------------------------------------------
// NOTE: These definitions should be safe to cast to and from the generic
//       radio_generic_packet_s structure as they all have the same header structure
//       and align to the same size.
//
//       Prefer to use these definitions across the codebase to ensure type
//       safety and consistency.
//
//       the generic contained should only be used to pack and unpack commands
//       into wire format for transmission over the radio.
// -----------------------------------------------------------------------------

#define _PAD_SIZE(partial_s) (PACKET_PAYLOAD_BYTES - sizeof(partial_s))

#define _COMMAND_NAME(name) cmd_##name##_s

#define _ASSERT_PACKET_SIZE(name)                                                             \
    BUILD_ASSERT(sizeof(struct _COMMAND_NAME(name)) == PACKET_SIZE, #name " size error")

// Packet with no payload data (all reserved)
#define MAKE_PACKET(name)                                                                     \
    struct _COMMAND_NAME(name)                                                                \
    {                                                                                         \
        struct packet_header_s hdr;                                                           \
        uint8_t _reserved[PACKET_PAYLOAD_BYTES];                                              \
    };                                                                                        \
    _ASSERT_PACKET_SIZE(name)

// Packet with defined payload data
#define MAKE_PACKET_WITH_PAYLOAD(name, partial_s)                                             \
    struct _COMMAND_NAME(name)                                                                \
    {                                                                                         \
        struct packet_header_s hdr;                                                           \
        partial_s payload;                                                                    \
        uint8_t _reserved[_PAD_SIZE(partial_s)];                                              \
    };                                                                                        \
    _ASSERT_PACKET_SIZE(name)

MAKE_PACKET(status_req);
MAKE_PACKET(abort);
MAKE_PACKET(ready);
MAKE_PACKET(arm);
MAKE_PACKET(fire);
MAKE_PACKET(launch_override);
MAKE_PACKET(fill_stop);
MAKE_PACKET(fill_resume);
MAKE_PACKET(manual_toggle);

MAKE_PACKET_WITH_PAYLOAD(ack, struct ack_s);
MAKE_PACKET_WITH_PAYLOAD(fill_exec, struct fill_exec_s);
MAKE_PACKET_WITH_PAYLOAD(status_rep, system_data_t);
MAKE_PACKET_WITH_PAYLOAD(manual_exec, struct manual_exec_s);

#undef _PAD_SIZE
#undef _COMMAND_NAME
#undef _ASSERT_PACKET_SIZE

#undef MAKE_PACKET
#undef MAKE_PACKET_WITH_PAYLOAD

// -----------------------------------------------------------------------------
// Serialization helpers (declarations)
// -----------------------------------------------------------------------------

enum pack_error_e
{
    PACK_ERROR_NONE = 0,
    PACK_ERROR_INVALID_ARG,
    PACK_ERROR_BUFFER_TOO_SMALL,
    PACK_ERROR_UNSUPPORTED_VERSION,
    PACK_ERROR_INVALID_CMD_ID,
};

enum pack_error_e packet_pack(const generic_packet_t *const packet, uint8_t *const out_buf,
                              const size_t out_buf_size);

enum pack_error_e packet_unpack(const uint8_t *const in_buf, const size_t in_buf_size,
                                generic_packet_t *const packet);

#endif // COMMANDS_H_
