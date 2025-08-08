#ifndef RADIO_COMMANDS_H_
#define RADIO_COMMANDS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Command IDs
// -----------------------------------------------------------------------------

enum radio_command_e {
    RCMD_NONE = 0,

    RCMD_STATUS_REQ = 1,
    RCMD_STATUS_REP,
    RCMD_ABORT,
    RCMD_READY,
    RCMD_ARM,
    RCMD_FIRE,
    RCMD_LAUNCH_OVERRIDE,
    RCMD_FILL_EXEC,
    RCMD_FILL_STOP,
    RCMD_FILL_RESUME,
    RCMD_MANUAL_TOGGLE,
    RCMD_MANUAL_EXEC,
    RCMD_ACK,

    RCMD_MAX
};

// -----------------------------------------------------------------------------
// Packet header (4 bytes)
// -----------------------------------------------------------------------------

// Always 1 byte fields, packed for on-the-wire transmission.
struct radio_cmd_header_s {
    uint8_t packet_version;
    uint8_t sender_id;
    uint8_t target_id;
    uint8_t command_id; // use values from enum radio_command_e
} __attribute__((packed));

// -----------------------------------------------------------------------------
// Global sizes
// -----------------------------------------------------------------------------

#define MAX_LORA_PACKET_BYTES       256u
#define RADIO_CMD_HEADER_BYTES      ((uint8_t)sizeof(struct radio_cmd_header_s))
#define RADIO_CMD_MAX_PAYLOAD_BYTES (MAX_LORA_PACKET_BYTES - RADIO_CMD_HEADER_BYTES)

// Generic payload container
// (variable length -- actual used length must be tracked by surrounding code)
struct radio_cmd_payload_s {
    uint8_t raw[RADIO_CMD_MAX_PAYLOAD_BYTES];
} __attribute__((packed));

// Top-level packet (header + payload bytes).
// NOTE: payload length is managed separately by application logic.
struct radio_cmd_s {
    struct radio_cmd_header_s hdr;
    struct radio_cmd_payload_s payload;
} __attribute__((packed));

// -----------------------------------------------------------------------------
// STATUS_REP payload layout
// -----------------------------------------------------------------------------
// NOTE: Ensure all 16 and 32 bit values align to 2 and 4 bytes boundaries respectively
// REVIEW: double check alignment

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

        // Filling station Quick Release
        uint16_t v_fill_qr0: 1;
        uint16_t v_fill_qr1: 1;

        // E-matches: ignition, drogue, main chute (3 bits)
        uint16_t ematch_ignition: 1;
        uint16_t ematch_drogue: 1;
        uint16_t ematch_main: 1;

        // remaining bits reserved for alignment
        uint16_t reserved: 3;
    };
    uint16_t raw;
} __attribute__((packed));

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
} __attribute__((packed));

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
} __attribute__((packed));

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
} __attribute__((packed));

union loadcell_weights_u { // FIXME: correct loadcell names
    struct {
        uint16_t loadcell1;
        uint16_t loadcell2;
        uint16_t loadcell3;
        uint16_t loadcell4;
    };
    uint16_t raw[4];
} __attribute__((packed));

// NOTE: Instead of using __attribute__((packet)) and memcpy, an alternative might be
//       to manually serialize/deserialized packets bit by bit, at the cost of some speed.
//
// REVIEW: field alignments
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
} __attribute__((packed));

// Compute size at compile time for a sanity check
#define STATUS_REP_SIZE sizeof(struct status_rep_s)
_Static_assert(STATUS_REP_SIZE <= RADIO_CMD_MAX_PAYLOAD_BYTES,
               "status_rep_s too large for payload");

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
} __attribute__((packed));

struct fill_pressure_params_s {
    uint16_t target_tank_deci_bar;
    uint16_t trigger_tank_deci_bar; // optional: set to 0xFFFF if unused
} __attribute__((packed));

struct fill_n2o_extra_s {
    uint16_t target_weight_grams;
    int16_t trigger_temp_deci_c;
} __attribute__((packed));

// Fill exec generic envelope
struct fill_exec_s {
    uint8_t program_id; // values from enum fill_program_e
    // variable parameters follow;
    // application MUST encode parameters sequentially according to selected program.
    // Maximum total payload must still fit.
    uint8_t params[];
} __attribute__((packed));

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
} __attribute__((packed));

struct manual_valve_ms_s {
    uint8_t valve_id;
    uint32_t duration_ms;
} __attribute__((packed));

struct manual_tare_s {
    uint8_t id; // loadcell or tank id
} __attribute__((packed));

// same logic as struct fill_exec_s
struct manual_exec_s {
    uint8_t manual_cmd_id;
    uint8_t params[];
} __attribute__((packed));

// -----------------------------------------------------------------------------
// ACK payload
// -----------------------------------------------------------------------------
// ACK contains the command id being acknowledged and optional error code(s).
// Define a compact representation.

struct ack_s {
    uint8_t ack_cmd_id;  // radio_command_t that is acknowledged
    uint8_t status_code; // 0 = OK, non-zero = error
    // optional error payload may follow (params[]), e.g., error details or manual cmd id
    uint8_t params[];
} __attribute__((packed));

// -----------------------------------------------------------------------------
// Serialization/Utility helpers (declarations)
// -----------------------------------------------------------------------------

// Note: implementations should handle endianness.

// Pack a status_rep_s into a provided buffer. Returns number of bytes written or negative on
// error (e.g., buffer too small).
int pack_status_rep(const struct status_rep_s *const src, uint8_t *const out_buf,
                    const size_t out_buf_size);

// Pack an ack packet into a provided buffer
int pack_ack(const struct ack_s *const src, uint8_t *const out_buf, const size_t out_buf_size);

int unpack_fill_exec(const struct fill_exec_s *const src, const size_t src_len,
                     uint8_t *const out_buf, const size_t out_buf_size);
int unpack_manual_exec(const struct manual_exec_s *const src, const size_t src_len,
                       uint8_t *const out_buf, const size_t out_buf_size);

#ifdef __cplusplus
}
#endif

#endif // RADIO_COMMANDS_H_
