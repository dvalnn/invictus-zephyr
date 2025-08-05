#ifndef HYDRA_H_
#define HYDRA_H_

#include "stdbool.h"
#include "stdint.h"

// NOTE: Solenoids are represented as modbus coils.
// Sensors are represented as modbus input registers.
// Temperatures in ºC. Pressures in Bar.

// TODO: implement is_connected functionality in the codebase.

// Upper Feed (UF) Hydraulic Regulation and Actuation (HYDRA) board structure
struct uf_hydra {
    uint8_t slave_id;
    bool is_connected;

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
#define N_COILS_UF 2

// Lower Feed (UF) Hydraulic Regulation and Actuation (HYDRA) board structure
struct lf_hydra {
    uint8_t slave_id;
    bool is_connected;

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
#define N_COILS_LF 2

// Rocket Hydras structure containing upper and lower feed hydras
struct rocket_hydras {
    struct uf_hydra uf;
    struct lf_hydra lf;
};

// Filling station hydra structure
struct fs_hydra {
    uint8_t slave_id;
    bool is_connected;

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
#define N_COILS_FS 6

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

struct hydra_sensor_read_desc {
    const char *const label;
    const uint8_t slave_id;
    const uint16_t start_addr;
    uint16_t *const dest;
    const uint16_t len;
    void (*on_error)(const char *const label, const int ret);
};

void hydras_sensor_read(const int client_iface, struct hydra_sensor_read_desc *const batch,
                        const uint8_t n_reads);

#define HYDRA_SENSOR_READ_DECL(label_str, slave, addr, buf, length, on_err)                   \
    (struct hydra_sensor_read_desc)                                                           \
    {                                                                                         \
        .label = label_str, .slave_id = slave, .start_addr = addr, .dest = buf,               \
        .len = length, .on_error = on_err                                                     \
    }

#define UF_HYDRA_SENSOR_READ(h, on_error)                                                     \
    HYDRA_SENSOR_READ_DECL("UF temperature", ((struct uf_hydra *)h)->slave_id,                \
                           CONFIG_HYDRA_UF_INPUT_ADDR_START,                                  \
                           &((struct uf_hydra *)h)->temperature, 1, on_error)

#define LF_HYDRA_SENSOR_READ(h, on_error)                                                     \
    HYDRA_SENSOR_READ_DECL("LF sensors", ((struct lf_hydra *)h)->slave_id,                    \
                           CONFIG_HYDRA_LF_INPUT_ADDR_START,                                  \
                           (uint16_t *const)((struct lf_hydra *)h)->sensors.raw,              \
                           ARRAY_SIZE(((struct lf_hydra *)h)->sensors.raw), on_error)

#define HYDRA_SENSOR_READ_BATCH_SIZE(batch) (sizeof(batch) / sizeof(batch[0]))

#endif // HYDRA_H_
