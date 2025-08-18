#ifndef LIFT_H_
#define LIFT_H_

#include "services/modbus/modbus.h"

#include <stdint.h>

// NOTE: E-Matches are represented as modbus coils.
// Load Cells are represented as modbus input registers.
// TODO: decide if weights should be signed, unsigned, or float.

// Filling Station (FS) Loadcell and Ignition Firing Terminal (LIFT) board structure
struct fs_lift {
    struct modbus_slave_metadata meta;
    uint16_t n2o_loadcell;
};

// Rocket (R) Loadcell and Ignition Firing Terminal (LIFT) board structure
struct rocket_lift {
    struct modbus_slave_metadata meta;

    union r_loadcells {
        struct {
            uint16_t loadcell1;
            uint16_t loadcell2;
            uint16_t loadcell3;
        } values;
        uint16_t raw[3];
    } loadcells;

    union r_ematches {
        struct {
            uint8_t main_ematch: 1;
            uint8_t _reserved: 7;
        };
        uint8_t raw;
    } ematches;
};

struct lift_boards {
    struct rocket_lift rocket;
    struct fs_lift fs;
};

/*
 * Initialize the lift structure with default values.
 * This function sets the slave IDs, connection states, e-match states, and sensor data to
 * their initial values.
 *
 * This function must be called before using the lift structure.
 * It expects that the slave IDs for the filling station (FS) and rocket (R)
 * lift boards are defined in the Kconfig options CONFIG_LIFT_FS_SLAVE_ID
 * and CONFIG_LIFT_R_SLAVE_ID, respectively.
 *
 * @param h Pointer to the lift structure to initialize.
 * @returns void.
 */
void lift_boards_init(struct lift_boards *const lb);

/**
 * Read the input registers from each lift board.
 * This function reads the input registers for the filling station and rocket lift
 * and updates their respective sensor values.
 *
 * @param client_iface The Modbus client interface to use for reading.
 * @param h Pointer to the lift structure containing the lift boards to read.
 * @returns void.
 *
 * @note This function flags the connection status of each lift board based on the read result.
 *       If the read is successful, it sets the `is_connected` flag to true.
 *       If the read fails, it sets the `is_connected` flag to false and logs a warning.
 */
void lift_boards_read_irs(const int client_iface, struct lift_boards *const lb);

#endif // LIFT_H_
