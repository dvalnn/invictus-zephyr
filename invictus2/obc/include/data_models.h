#ifndef DATA_MODELS_H_
#define DATA_MODELS_H_

#include <stdint.h>

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

struct rocket_state_s {
    uint8_t major; // Values from enum rocket_state_e
    uint8_t minor; // Values from enum rocket_state_flight_e or enum rocket_state_fill_e
};

struct full_system_data_s {
    struct rocket_state_s rocket_state;
    union pressures_u pressures;
    union thermocouples_u thermocouples;
    union actuators_bitmap_u actuators;
    union loadcell_weights_u loadcells;
    struct navigator_sensors_s navigator;
};

enum state_e {
    _RS_NONE = 0,
    RS_ROOT,
    RS_IDLE,
    RS_FILLING,
    RS_ABORT,
    RS_READY,
    RS_ARMED,
    RS_FLIGHT,

    _RS_MAX
};

// Rocket Flight Substates
enum rocket_state_flight_e {
    _RS_FLIGHT_NONE = 0,

    RS_FLIGHT_LAUNCH,
    RS_FLIGHT_ASCENT,
    RS_FLIGHT_APOGEE,
    RS_FLIGHT_DROGE_CHUTE,
    RS_FLIGHT_MAIN_CHUTE,
    RS_FLIGHT_TOUCHDOWN,

    _RS_FLIGHT_MAX
};

// Rocket Filling Substates
enum rocket_state_fill_e {
    _RS_FILL_NONE = 0,

    RS_FILL_SAFE_PAUSE,
    RS_FILL_SAFE_PAUSE_IDLE,
    RS_FILL_SAFE_PAUSE_VENT,

    RS_FILL_FILLING_N2,
    RS_FILL_FILLING_N2_IDLE,
    RS_FILL_FILLING_N2_FILL,

    RS_FILL_PRE_PRESS,
    RS_FILL_PRE_PRESS_IDLE,
    RS_FILL_PRE_PRESS_VENT,
    RS_FILL_PRE_PRESS_FILL_N2,

    RS_FILL_FILLING_N2O,
    RS_FILL_FILLING_N2O_IDLE,
    RS_FILL_FILLING_N2O_FILL,
    RS_FILL_FILLING_N2O_VENT,

    RS_FILL_POST_PRESS,
    RS_FILL_POST_PRESS_IDLE,
    RS_FILL_POST_PRESS_VENT,
    RS_FILL_POST_PRESS_FILL_N2,

    _RS_FILL_MAX
};

#endif // DATA_MODELS_H_
