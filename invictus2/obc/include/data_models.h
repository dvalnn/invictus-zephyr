#ifndef DATA_MODELS_H_
#define DATA_MODELS_H_

#include <stdint.h>

typedef enum {
    ROOT,
    IDLE,
    FILLING,
    READY,
    ARMED,
    FLIGHT,
    ABORT,
} main_state_t;

// Filling Substates
typedef enum {
    SAFE_PAUSE = ABORT + 1,
    SAFE_PAUSE_IDLE,
    SAFE_PAUSE_VENT,

    FILL_N2,
    FILL_N2_IDLE,
    FILL_N2_FILL,
    FILL_N2_VENT,

    PRE_PRESS,
    PRE_PRESS_IDLE,
    PRE_PRESS_VENT,
    PRE_PRESS_FILL_N2,

    FILL_N2O,
    FILL_N2O_IDLE,
    FILL_N2O_FILL,
    FILL_N2O_VENT,

    POST_PRESS,
    POST_PRESS_IDLE,
    POST_PRESS_VENT,
    POST_PRESS_FILL_N2,
} filling_state_t;


// Flight Substates
typedef enum {
    IGNITION = POST_PRESS_FILL_N2 + 1,
    BOOST,
    COAST,
    APOGEE,
    DROGUE_CHUTE,
    MAIN_CHUTE,
    TOUCHDOWN,
} flight_state_t;

#define STATE_COUNT (TOUCHDOWN + 1)

typedef enum {
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
typedef union {
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
} actuators_bitmap_t;

typedef union {
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
} thermocouples_t;

typedef union {
    struct {
        uint16_t n2o_tank_pressure;
        uint16_t chamber_pressure;
        uint16_t n2o_line_pressure;
        uint16_t n2_line_pressure;
        uint16_t quick_dc_pressure;
    };
    uint16_t raw[6];
} pressures_t;

typedef union {
    struct {
        uint16_t n2o_loadcell;  // N2O bottle loadcell
        uint16_t rail_loadcell; // Only used during competition to measure rocket weight

        // Loadcells for static tests
        uint16_t thrust_loadcell1;
        uint16_t thrust_loadcell2;
        uint16_t thrust_loadcell3;
    };
    uint16_t raw[5];
} loadcell_weights_t;

typedef struct {
    // GPS:
    uint32_t gps_latitude_u32;  // reinterpret-casted float bits
    uint32_t gps_longitude_u32; // reinterpret-casted float bits
    uint16_t gps_altitude;      // altitude in meters
    uint16_t gps_hspeed;        // horizontal speed (units TBD)
    uint8_t gps_sats;           // gps number of satelites
    uint8_t _reserved;          // extra byte for alignment

    // Barometers:
    uint16_t baro1, baro2;

    // IMU:
    int16_t mag_x, mag_y, mag_z; // Magnetometer
    int16_t gyro_x, gyro_y, gyro_z; // Gyroscope
    int16_t accel_x, accel_y, accel_z; // Accelerometer
} navigator_sensors_t;

typedef struct {
    int16_t vertical_speed;
    int16_t vertical_acceleration;
    int16_t altitude;
    int16_t max_altitude;
    int16_t quaternions[4];
} kalman_data_t;

typedef struct state_data_s {
    main_state_t main_state;
    filling_state_t filling_state;
    flight_state_t flight_state;
} state_data_t;

typedef struct {
    state_data_t state;
    pressures_t pressures;
    thermocouples_t thermocouples;
    actuators_bitmap_t actuators;
    loadcell_weights_t loadcells;
    navigator_sensors_t navigator;
    kalman_data_t kalman;
} system_data_t;

#endif // DATA_MODELS_H_
