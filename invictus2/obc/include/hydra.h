#ifndef HYDRA_H_
#define HYDRA_H_

#include "stdbool.h"
#include "stdint.h"
#include "zephyr/toolchain.h"

struct uf_hydra {
    int slave_id;      // Slave ID for the hydra on the modbus bus
    bool is_connected; // True if the hydra is connected

    union uf_solenoids {
        struct {
            bool vsl2_valve_solenoid;
            bool vsv2_vent_solenoid;
        } solenoid_states;

        uint16_t raw_states; // Raw state representation of the solenoids
    } solenoids;

    uint16_t uf_temperature; // Upper feed temperature probe temperature in ºC
};

struct lf_hydra {
    int slave_id;
    bool is_connected;

    union lf_solenoids {
        struct {
            bool vsa1_abort_solenoid;
            bool vsl1_main_solenoid;
        } solenoid_states;

        uint16_t raw_states; // Raw state representation of the solenoids
    } solenoids;

    union lf_sensors {
        struct {
            uint16_t lf_temperature; // Lower feed temperature probe temperature in ºC
            uint16_t lf_pressure;    // Lower feed pressure in bar
            uint16_t cc_pressure;    // Combustion chamber pressure in bar
        } data;

        uint16_t raw_data[3]; // Raw data representation of the sensors
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
inline void hydras_init(struct hydras *h)
{
#if defined(CONFIG_HYDRA_UF_SLAVE_ID) && defined(CONFIG_HYDRA_LF_SLAVE_ID)
    BUILD_ASSERT(CONFIG_HYDRA_UF_SLAVE_ID != CONFIG_HYDRA_LF_SLAVE_ID,
                 "UF and LF hydras must have different slave IDs.");

    h->uf.slave_id = CONFIG_HYDRA_UF_SLAVE_ID;
    h->lf.slave_id = CONFIG_HYDRA_LF_SLAVE_ID;
#else
    BUILD_ASSERT(false, "UF and LF hydras must have slave IDs defined."
                        "Use CONFIG_HYDRA_UF_SLAVE_ID and CONFIG_HYDRA_LF_SLAVE_ID"
                        "Kconfig options in your prj.conf file value.");
#endif

    h->uf.is_connected = false;
    h->lf.is_connected = false;

    // Initialize solenoids to default states
    h->uf.solenoids.raw_states = 0;
    h->lf.solenoids.raw_states = 0;

    // Initialize sensors to default values
    h->lf.sensors.raw_data[0] = 0; // lf_temperature
    h->lf.sensors.raw_data[1] = 0; // lf_pressure
    h->lf.sensors.raw_data[2] = 0; // cc_pressure

    // Initialize temperatures
    h->uf.uf_temperature = 0;
}

inline bool hydras_connected(const struct hydras *h)
{
    return h->uf.is_connected && h->lf.is_connected;
}

inline bool hydras_is_uf_connected(const struct hydras *h)
{
    return h->uf.is_connected;
}

inline bool hydras_is_lf_connected(const struct hydras *h)
{
    return h->lf.is_connected;
}

#endif // HYDRA_H_
