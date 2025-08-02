#ifndef HYDRA_H_
#define HYDRA_H_

#include "stdbool.h"
#include "stdint.h"

struct uf_hydra {
    int slave_id;      // Slave ID for the hydra on the modbus bus
    bool is_connected; // True if the hydra is connected

    union uf_solenoids {
        struct {
            bool vsl2_valve_solenoid;
            bool vsv2_vent_solenoid;
        } solenoid_states;

        uint16_t raw; // Raw state representation of the solenoids
    } solenoids;

    uint16_t temperature; // Upper feed temperature probe temperature in ºC
};

struct lf_hydra {
    int slave_id;
    bool is_connected;

    union lf_solenoids {
        struct {
            bool vsa1_abort_solenoid;
            bool vsl1_main_solenoid;
        } solenoid_states;

        uint16_t raw; // Raw state representation of the solenoids
    } solenoids;

    union lf_sensors {
        struct {
            uint16_t lf_temperature; // Lower feed temperature probe temperature in ºC
            uint16_t lf_pressure;    // Lower feed pressure in bar
            uint16_t cc_pressure;    // Combustion chamber pressure in bar
        };

        uint16_t raw[3]; // Raw data representation of the sensors
    } sensors;
};

struct hydras {
    struct uf_hydra uf;
    struct lf_hydra lf;
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
void hydras_init(struct hydras *h);

/*
 * Read data from the hydras using Modbus RTU.
 * This function reads the input registers for the upper feed (UF) and lower feed (LF hydras)
 * and updates their respective structures.
 *
 * @param h Pointer to the hydras structure to read data into.
 * @param client_iface The Modbus client interface to use for communication.
 *
 * @returns 0 on success, negative error code on failure.
 */
int hydras_modbus_read(struct hydras *h, const int client_iface);

bool hydras_connected(const struct hydras *h);

bool hydras_is_uf_connected(const struct hydras *h);

bool hydras_is_lf_connected(const struct hydras *h);

#endif // HYDRA_H_
