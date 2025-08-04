#ifndef HYDRA_H_
#define HYDRA_H_

#include "stdbool.h"
#include "stdint.h"

struct uf_hydra {
    int slave_id; // Slave ID for the hydra on the modbus bus
    // TODO: Do something about this
    bool is_connected; // True if the hydra is connected

    struct uf_solenoids {
        bool vsl2_valve;
        bool vsv2_vent;
    } solenoids;

    uint16_t temperature; // Upper feed temperature probe temperature in ºC
};

struct lf_hydra {
    int slave_id;
    // TODO: Do something about this
    bool is_connected;

    struct lf_solenoids {
        bool vsa1_abort;
        bool vsl1_main;
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

struct rocket_hydras {
    struct uf_hydra uf;
    struct lf_hydra lf;
};

struct fs_hydra {
    int slave_id;
    bool is_connected;

    // TODO: Check Names
    struct fs_solenoids {
        bool quick_dc_t1;
        bool quick_dc_t2;
        bool vent_t1;
        bool vent_t2;
        bool generic_valve_3;
        bool generic_valve_4;
    } solenoids;

    union fs_sensors {
        struct {
            uint16_t temperature; // Filling station temperature in ºC
            uint16_t pressure_1;  // Filling station pressure in bar
            uint16_t pressure_2;  // Filling station pressure in bar
        };

        uint16_t raw[3]; // Raw data representation of the sensors
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
int rocket_hydras_modbus_read(struct rocket_hydras *h, const int client_iface);

#endif // HYDRA_H_
