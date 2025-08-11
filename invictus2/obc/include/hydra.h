#ifndef HYDRA_H_
#define HYDRA_H_

#include "services/modbus.h"

#include <stdint.h>

// NOTE: Solenoids are represented as modbus coils.
// Sensors are represented as modbus input registers.
// Temperatures in ºC. Pressures in Bar.

// Upper Feed (UF) Hydraulic Regulation and Actuation (HYDRA) board structure
struct uf_hydra {
    struct modbus_slave_metadata meta;

    union uf_solenoids {
        struct {
            uint8_t pressurizing_valve: 1;
            uint8_t venting_valve: 1;
            uint8_t _reserved: 6;
        };
        uint8_t raw;
    } solenoids;

    union uf_sensors {
        struct {
            uint16_t uf_temperature1; 
            uint16_t uf_temperature2;
            uint16_t uf_temperature3;
        };
        uint16_t raw[3];
    } sensors;
};

// Lower Feed (UF) Hydraulic Regulation and Actuation (HYDRA) board structure
struct lf_hydra {
    struct modbus_slave_metadata meta;

    union lf_solenoids {
        // REVIEW: change these names once there is a better naming convention
        struct {
            uint8_t abort_valve: 1;
            uint8_t main_valve: 1;
            uint8_t _reserved: 6;
        };
        uint8_t raw;
    } solenoids;

    union lf_sensors {
        struct {
            uint16_t lf_temperature1;
            uint16_t lf_temperature2;
            uint16_t lf_pressure;
            uint16_t cc_pressure;
        };
        uint16_t raw[4];
    } sensors;
};

// Rocket Hydras structure containing upper and lower feed hydras
struct rocket_hydras {
    struct uf_hydra uf;
    struct lf_hydra lf;
};

// Filling station hydra structure
struct fs_hydra {
    struct modbus_slave_metadata meta;

    union fs_solenoids {
        struct {
            uint8_t n2o_fill_valve: 1;
            uint8_t n2_fill_valve: 1;
            uint8_t n2o_purge_valve: 1;
            uint8_t n2_purge_valve: 1;
            uint8_t n2o_quick_dc: 1;
            uint8_t n2_quick_dc: 1;
            uint8_t _reserved: 2;
        };
        uint8_t raw;
    } solenoids;

    union fs_sensors {
        struct {
            // Pressure sensors
            uint16_t n2o_pressure;
            uint16_t n2_pressure;
            uint16_t quick_dc_pressure;

            // Temperature sensors
            uint16_t n2o_temperature1; // temperature before solenoid
            uint16_t n2o_temperature2; // temperature after solenoid
            uint16_t n2_temperature;
        };
        uint16_t raw[6];
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
