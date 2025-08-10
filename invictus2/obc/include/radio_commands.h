#ifndef RADIO_COMMANDS_H_
#define RADIO_COMMANDS_H_

#include "zephyr/toolchain.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// REVIEW: make this KConfig param?
#define SUPPORTED_PACKET_VERSION 1

// -----------------------------------------------------------------------------
// Packet header
// -----------------------------------------------------------------------------

// Always 1 byte fields, packed for on-the-wire transmission.
struct cmd_header_s {
    uint8_t packet_version;
    uint8_t sender_id;
    uint8_t target_id;
    uint8_t command_id; // use values from enum radio_command_e
};

// -----------------------------------------------------------------------------
// Generic Command Structure
// -----------------------------------------------------------------------------

#define RADIO_CMD_SIZE          128u
#define RADIO_CMD_HEADER_BYTES  ((uint8_t)sizeof(struct cmd_header_s))
#define RADIO_CMD_PAYLOAD_BYTES (RADIO_CMD_SIZE - RADIO_CMD_HEADER_BYTES)

// NOTE: Generic commands structure, cast to a specific command type to access payloads.
struct radio_generic_cmd_s {
    struct cmd_header_s header;
    uint8_t _reserved[RADIO_CMD_PAYLOAD_BYTES];
};

// -----------------------------------------------------------------------------
// STATUS_REP payload layout
// -----------------------------------------------------------------------------

// Actuators bitfield definition (13 bits -> store in 2 bytes)
union actuators_bits_u { // FIXME: correct actuator names
    struct {
        // Rocket valves
        uint16_t v_rocket_valve0: 1;
        uint16_t v_rocket_valve1: 1;
        uint16_t v_rocket_valve2: 1;
        uint16_t v_rocket_valve3: 1;

        // Filling station valves
        uint16_t v_fill_valve0: 1;
        uint16_t v_fill_valve1: 1;
        uint16_t v_fill_valve2: 1;
        uint16_t v_fill_valve3: 1;

        // E-matches: ignition, drogue, main chute (3 bits)
        uint16_t ematch_ignition: 1;
        uint16_t ematch_drogue: 1;
        uint16_t ematch_main: 1;

        // Filling station Quick Release
        //  NOTE : After launch these should be interpreted
        //       as reserved bits as their state is no longer
        //       relevant.
        uint16_t v_fill_qr0: 1;
        uint16_t v_fill_qr1: 1;

        // remaining bits reserved for alignment
        uint16_t reserved: 3;
    };
    uint16_t raw;
};

struct navigator_sensors_s {
    uint32_t gps_latitude_u32;  // reinterpret-casted float bits
    uint32_t gps_longitude_u32; // reinterpret-casted float bits
    uint16_t gps_altitude;      // altitude in meters
    uint16_t gps_hspeed;        // horizontal speed (units TBD)
    uint8_t gps_sats;           // gps number of satelites
    uint8_t _reserved;          // extra byte for alignment

    // NAV Sensors:
    uint16_t baro_altitude[2]; // Bar Altitude
    int16_t mag[3];            // Mag XYZ
    int16_t imu_gyr[3];        // IMU Gyr XYZ
    int16_t imu_accel[3];      // IMU Accel XYZ sensors/samples

    // Kalman NAV (16 bytes total): vel_z, accel_z, alt, max_alt (each int16),
    // then 4 quaternions as int16 each
    int16_t kalman_vel_z;
    int16_t kalman_accel_z;
    int16_t kalman_alt;
    int16_t kalman_max_alt;
    int16_t kalman_quat[4];
};

union thermocouples_u { // FIXME: correct thermo names
    struct {
        int16_t thermo1;
        int16_t thermo2;
        int16_t thermo3;
        int16_t thermo4;
        int16_t thermo5;
        int16_t thermo6;
        int16_t thermo7;
        int16_t thermo8;
    };
    int16_t raw[8];
};

union pressures_u { // FIXME: correct pressure names
    struct {
        uint16_t pressure1;
        uint16_t pressure2;
        uint16_t pressure3;
        uint16_t pressure4;
        uint16_t pressure5;
        uint16_t pressure6;
    };
    uint16_t raw[6];
};

union loadcell_weights_u { // FIXME: correct loadcell names
    struct {
        uint16_t loadcell1;
        uint16_t loadcell2;
        uint16_t loadcell3;
        uint16_t loadcell4;
    };
    uint16_t raw[4];
};

// NOTE: Alignment was manually checked. It might be usable cross-platform even without
//       compiler-specific attributes, as long as the fields are defined in the same order
//       and sizes are consistent across platforms.
//
//       For our use case, however, both the sender and receiver are the same platform
//       and compiler, so we can safely use memcpy to serialize and deserialize.
//
// NOTE:                                                 byte alignments
struct status_rep_s {                     // byte block | 2 byte block | 4 byte block |
    uint8_t rocket_state;                 //     1      |      0.5     |     0.25     |
    uint8_t rocket_substate;              //     2      |       1      |     0.50     |
    union pressures_u pressures;          //     14     |       7      |     3.50     |
    union thermocouples_u thermocouples;  //     30     |       15     |     7.50     |
    union actuators_bits_u actuators;     //     32     |       16     |     8.00     |
    union loadcell_weights_u loadcells;   //     40     |       20     |     10.0     |
    struct navigator_sensors_s navigator; //     92     |       46     |     23.0     |
};

// -----------------------------------------------------------------------------
// Fill Exec payloads
// -----------------------------------------------------------------------------

enum fill_program_e {
    FILL_PROGRAM_NONE = 0,
    FILL_PROGRAM_COPV = 1,
    FILL_PROGRAM_N_PRE,
    FILL_PROGRAM_N2O,
    FILL_PROGRAM_N_POST,
};

struct fill_copv_params_s {
    uint16_t target_copv_deci_bar;
};

struct fill_pressure_params_s {
    uint16_t target_tank_deci_bar;
    uint16_t trigger_tank_deci_bar; // optional: set to 0xFFFF if unused
};

struct fill_n2o_extra_s {
    uint16_t target_weight_grams;
    int16_t trigger_temp_deci_c;
};

// Fill exec generic envelope
struct fill_exec_s {
    uint8_t program_id;                          // values from enum fill_program_e
    uint8_t params[RADIO_CMD_PAYLOAD_BYTES - 1]; // 1 byte for program_id, rest for params
};

#define FILL_COPV_PARAM_BYTES     (sizeof(struct fill_copv_params_s))
#define FILL_PRESSURE_PARAM_BYTES (sizeof(struct fill_pressure_params_s))
#define FILL_N2O_EXTRA_BYTES      (sizeof(struct fill_n2o_extra_s))

// -----------------------------------------------------------------------------
// Manual Commands
// -----------------------------------------------------------------------------

enum manual_cmd_e {
    MANUAL_CMD_NONE = 0,
    MANUAL_CMD_SD_LOG_START,
    MANUAL_CMD_SD_LOG_STOP,
    MANUAL_CMD_SD_STATUS,
    MANUAL_CMD_VALVE_STATE,
    MANUAL_CMD_VALVE_MS,
    MANUAL_CMD_LOADCELL_TARE,
    MANUAL_CMD_TANK_TARE,
};

struct manual_valve_state_s {
    uint8_t valve_id;
    uint8_t open; // boolean: 0 closed, 1 open
};

struct manual_valve_ms_s {
    uint8_t valve_id;
    uint32_t duration_ms;
};

struct manual_tare_s {
    uint8_t id; // loadcell or tank id
};

// same logic as struct fill_exec_s
struct manual_exec_s {
    uint8_t manual_cmd_id;
    uint8_t params[RADIO_CMD_PAYLOAD_BYTES - 1];
};

// -----------------------------------------------------------------------------
// ACK payload
// -----------------------------------------------------------------------------
// ACK contains the command id being acknowledged and optional error code(s).
// Define a compact representation.

struct ack_s {
    uint8_t ack_cmd_id;  // radio_command_t that is acknowledged
    uint8_t status_code; // 0 = OK, non-zero = error
    uint8_t params[RADIO_CMD_PAYLOAD_BYTES - 2];
};

// -----------------------------------------------------------------------------
// Command IDs
// -----------------------------------------------------------------------------
enum radio_command_e {
    RCMD_NONE = 0,

    // No Payload Commands
    RCMD_STATUS_REQ = 1,
    RCMD_ABORT,
    RCMD_READY,
    RCMD_ARM,
    RCMD_FIRE,
    RCMD_LAUNCH_OVERRIDE,
    RCMD_FILL_STOP,
    RCMD_FILL_RESUME,
    RCMD_MANUAL_TOGGLE,

    // Payload Commands
    RCMD_STATUS_REP,
    RCMD_FILL_EXEC,
    RCMD_MANUAL_EXEC,
    RCMD_ACK,

    RCMD_MAX
};

// -----------------------------------------------------------------------------
// Individual Command Definitions
// -----------------------------------------------------------------------------
// NOTE: These definitions should be safe to cast to and from the generic
//       radio_generic_cmd_s structure as they all have the same header structure
//       and align to the same size.
//
//       Prefer to use these definitions across the codebase to ensure type
//       safety and consistency.
//
//       the generic contained should only be used to pack and unpack commands
//       into wire format for transmission over the radio.
// -----------------------------------------------------------------------------

#define _PAD_SIZE(partial_s) (RADIO_CMD_PAYLOAD_BYTES - sizeof(partial_s))

#define _COMMAND_NAME(name) cmd_##name##_s

#define _ASSERT_CMD_SIZE(name)                                                                \
    BUILD_ASSERT(sizeof(struct _COMMAND_NAME(name)) == RADIO_CMD_SIZE, #name " size error")

// Command with no payload data (all reserved)
#define MAKE_CMD(name)                                                                        \
    struct _COMMAND_NAME(name) {                                                              \
        struct cmd_header_s hdr;                                                              \
        uint8_t _reserved[RADIO_CMD_PAYLOAD_BYTES];                                           \
    };                                                                                        \
    _ASSERT_CMD_SIZE(name)

// Command with defined payload data
#define MAKE_CMD_WITH_PAYLOAD(name, partial_s)                                                \
    struct _COMMAND_NAME(name) {                                                              \
        struct cmd_header_s hdr;                                                              \
        partial_s payload;                                                                    \
        uint8_t _reserved[_PAD_SIZE(partial_s)];                                              \
    };                                                                                        \
    _ASSERT_CMD_SIZE(name)

MAKE_CMD(status_req);
MAKE_CMD(abort);
MAKE_CMD(ready);
MAKE_CMD(arm);
MAKE_CMD(fire);
MAKE_CMD(launch_override);
MAKE_CMD(fill_stop);
MAKE_CMD(fill_resume);
MAKE_CMD(manual_toggle);

MAKE_CMD_WITH_PAYLOAD(ack, struct ack_s);
MAKE_CMD_WITH_PAYLOAD(fill_exec, struct fill_exec_s);
MAKE_CMD_WITH_PAYLOAD(status_rep, struct status_rep_s);
MAKE_CMD_WITH_PAYLOAD(manual_exec, struct manual_exec_s);

#undef _PAD_SIZE
#undef _COMMAND_NAME
#undef _ASSERT_CMD_SIZE

#undef MAKE_CMD
#undef MAKE_CMD_WITH_PAYLOAD

// -----------------------------------------------------------------------------
// Serialization helpers (declarations)
// -----------------------------------------------------------------------------

enum pack_error_e {
    PACK_ERROR_NONE = 0,
    PACK_ERROR_INVALID_ARG,
    PACK_ERROR_BUFFER_TOO_SMALL,
    PACK_ERROR_UNSUPPORTED_VERSION,
    PACK_ERROR_INVALID_CMD_ID,
};

enum pack_error_e radio_cmd_pack(const struct radio_generic_cmd_s *const cmd,
                                 uint8_t *const out_buf, const size_t out_buf_size);

enum pack_error_e radio_cmd_unpack(const uint8_t *const in_buf, const size_t in_buf_size,
                                   struct radio_generic_cmd_s *const cmd);

#endif // RADIO_COMMANDS_H_
