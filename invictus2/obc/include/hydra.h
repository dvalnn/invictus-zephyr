#ifndef HYDRA_H_
#define HYDRA_H_

#include "stdint.h"

// NOTE: Solenoids are represented as modbus coils.
// Sensors are represented as modbus input registers.
// Temperatures in ºC. Pressures in Bar.

// TODO: implement is_connected functionality in the codebase.

struct hydra_metadata {
    uint16_t ir_start;    // Start address for input registers
    uint8_t slave_id;     // Modbus slave ID
    uint8_t is_connected; // Connection status
};

// Upper Feed (UF) Hydraulic Regulation and Actuation (HYDRA) board structure
struct uf_hydra {
    struct hydra_metadata meta;

    union uf_solenoids {
        // REVIEW: change these names once there is a better naming convention
        struct {
            uint8_t vsl2_valve: 1;
            uint8_t vsv2_vent: 1;
            uint8_t _reserved: 6;
        };
        uint8_t raw;
    } solenoids;

    // REVIEW: There is a change this hydra has a triple temperature probe
    //  In that case, this may need to be changed to an union with a struct
    uint16_t temperature;
};

// Lower Feed (UF) Hydraulic Regulation and Actuation (HYDRA) board structure
struct lf_hydra {
    struct hydra_metadata meta;

    union lf_solenoids {
        // REVIEW: change these names once there is a better naming convention
        struct {
            uint8_t vsa1_abort: 1;
            uint8_t vsl1_main: 1;
            uint8_t _reserved: 6;
        };
        uint8_t raw;
    } solenoids;

    union lf_sensors {
        struct {
            uint16_t lf_temperature;
            uint16_t lf_pressure;
            uint16_t cc_pressure;
        };
        uint16_t raw[3];
    } sensors;
};

// Rocket Hydras structure containing upper and lower feed hydras
struct rocket_hydras {
    struct uf_hydra uf;
    struct lf_hydra lf;
};

// Filling station hydra structure
struct fs_hydra {
    struct hydra_metadata meta;

    union fs_solenoids {
        struct {
            uint8_t enable_N2O: 1;
            uint8_t enable_N2: 1;
            uint8_t vent_N2O: 1;
            uint8_t vent_N2: 1;
            uint8_t quick_dc_t1: 1;
            uint8_t quick_dc_t2: 1;
            uint8_t _reserved: 2;
        };
        uint8_t raw;
    } solenoids;

    union fs_sensors {
        struct {
            // Pressure sensors
            uint16_t pressure_N2O;
            uint16_t pressure_N2;
            uint16_t pressure_quick_dc;

            // Temperature sensors
            uint16_t temp_N2_probe1;
            uint16_t temp_N2_probe2;
        };
        uint16_t raw[5];
    } sensors;
};

/*
 * Initialize the hydras structure with default values.
 * This function sets the slave IDs, connection states, solenoid states, and sensor data to
 * their initial values.
 *
 * This function must be called before using the hydras structure.
 * It expects that the slave IDs for the upper feed (UF) and lower feed (LF)
 * hydras are defined in the Kconfig options CONFIG_HYDRA_UF_SLAVE_ID
 * and CONFIG_HYDRA_LF_SLAVE_ID, respectively.
 *
 * @param h Pointer to the hydras structure to initialize.
 * @returns void.
 */
void rocket_hydras_init(struct rocket_hydras *h);

/**
 * Read the sensors from the hydras.
 * This function reads the input registers for the upper feed and lower feed hydras
 * and updates their respective sensor values.
 *
 * @param client_iface The Modbus client interface to use for reading.
 * @param h Pointer to the hydras structure containing the hydras to read.
 * @returns void.
 *
 * @note This function flags the connection status of each hydra based on the read result.
 *       If the read is successful, it sets the `is_connected` flag to true.
 *       If the read fails, it sets the `is_connected` flag to false and logs a warning.
 */
void rocket_hydras_sensor_read(const int client_iface, struct rocket_hydras *const h);

// TODO: implement this function
void rocket_hydras_coils_read(const int client_iface, struct rocket_hydras *const h);

#endif // HYDRA_H_
