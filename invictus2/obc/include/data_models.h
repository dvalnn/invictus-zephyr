#ifndef DATA_MODELS_H_
#define DATA_MODELS_H_

#include <stdint.h>

#include "services/state_machine/main_sm.h"

typedef enum main_state_e {
    ROOT,
    IDLE,
    FILLING,
    READY,
    ARMED,
    FLIGHT,
    ABORT,
} main_state_t;

// Filling Substates
typedef enum filling_state_e {
    FILL_SAFE_PAUSE,
    FILL_SAFE_PAUSE_IDLE,
    FILL_SAFE_PAUSE_VENT,

    FILL_FILLING_N2,
    FILL_FILLING_N2_IDLE,
    FILL_FILLING_N2_FILL,

    FILL_PRE_PRESS,
    FILL_PRE_PRESS_IDLE,
    FILL_PRE_PRESS_VENT,
    FILL_PRE_PRESS_FILL_N2,

    FILL_FILLING_N2O,
    FILL_FILLING_N2O_IDLE,
    FILL_FILLING_N2O_FILL,
    FILL_FILLING_N2O_VENT,

    FILL_POST_PRESS,
    FILL_POST_PRESS_IDLE,
    FILL_POST_PRESS_VENT,
    FILL_POST_PRESS_FILL_N2,

    _FILL_MAX
} filling_state_t;


// Flight Substates
typedef enum flight_state_e {
    FLIGHT_LAUNCH,
    FLIGHT_ASCENT,
    FLIGHT_APOGEE,
    FLIGHT_DROGUE_CHUTE,
    FLIGHT_MAIN_CHUTE,
    FLIGHT_TOUCHDOWN,

    _FLIGHT_MAX
} flight_state_t;

typedef enum valve_e {
    _VALVE_NONE = 0,

    VALVE_N2O_FILL,
    VALVE_N2O_PURGE,
    VALVE_N2_FILL,
    VALVE_N2_PURGE,
    VALVE_N2O_QUICK_DC,
    VALVE_N2_QUICK_DC,
    VALVE_PRESSURIZING,
    VALVE_MAIN,
    VALVE_VENT,
    VALVE_ABORT,

    _VALVE_COUNT,
} valve_t;


// Structures and unions for various sensor data and actuator states
// Used across the ZBUS channels.

// Actuators bitfield definition (13 bits -> store in 2 bytes)
union actuators_bitmap_u {
    struct {
        // Rocket valves
        uint16_t v_pressurizing: 1;
        uint16_t v_venting: 1;
        uint16_t v_abort: 1;
        uint16_t v_main: 1;

        // Filling station valves
        uint16_t v_n2o_fill: 1;
        uint16_t v_n2o_purge: 1;
        uint16_t v_n2_fill: 1;
        uint16_t v_n2_purge: 1;

        // E-matches: ignition, drogue, main chute (3 bits)
        uint16_t ematch_ignition: 1;
        uint16_t ematch_drogue: 1;
        uint16_t ematch_main: 1;

        // Filling station Quick Release
        //  NOTE : After launch these should be interpreted
        //       as reserved bits as their state is no longer
        //       relevant.
        uint16_t v_n2o_quick_dc: 1;
        uint16_t v_n2_quick_dc: 1;

        // remaining bits reserved for alignment
        uint16_t reserved: 3;
    };
    uint16_t raw;
};

union thermocouples_u {
    struct {
        int16_t n2o_tank_uf_t1;
        int16_t n2o_tank_uf_t2;
        int16_t n2o_tank_uf_t3;
        int16_t n2o_tank_lf_t1;
        int16_t n2o_tank_lf_t2;
        int16_t chamber_thermo;
        int16_t n2o_line_thermo1; // before and after solenoids
        int16_t n2o_line_thermo2;
        int16_t n2_line_thermo;
    };
    int16_t raw[8];
};

union pressures_u {
    struct {
        uint16_t n2o_tank_pressure;
        uint16_t chamber_pressure;
        uint16_t n2o_line_pressure;
        uint16_t n2_line_pressure;
        uint16_t quick_dc_pressure;
    };
    uint16_t raw[6];
};

union loadcell_weights_u {
    struct {
        uint16_t n2o_loadcell;  // N2O bottle loadcell
        uint16_t rail_loadcell; // Only used during competition to measure rocket weight

        // Loadcells for static tests
        uint16_t thrust_loadcell1;
        uint16_t thrust_loadcell2;
        uint16_t thrust_loadcell3;
    };
    uint16_t raw[5];
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

typedef struct state_data_s {
    main_state_t main_state;
    filling_state_t filling_state;
    flight_state_t flight_state;
    sm_state_t sm_state;
} state_data_t;

struct full_system_data_s {
    state_data_t state;
    union pressures_u pressures;
    union thermocouples_u thermocouples;
    union actuators_bitmap_u actuators;
    union loadcell_weights_u loadcells;
    struct navigator_sensors_s navigator;
};

#endif // DATA_MODELS_H_
