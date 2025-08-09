#ifndef LIFT_H_
#define LIFT_H_

#include "stdint.h"

// NOTE: E-Matches are represented as modbus coils.
// Load Cells are represented as modbus input registers.
// TODO: decide if weights should be signed, unsigned, or float.

// TODO: implement is_connected functionality in the codebase.

struct lift_metadata {
    uint16_t ir_start;    // Start address for input registers
    uint8_t slave_id;     // Modbus slave ID
    uint8_t is_connected; // Connection status
};

// Filling Station (FS) Loadcell and Ignition Firing Terminal (LIFT) board structure
struct fs_lift {
    struct lift_metadata meta;

    // Note: the board has 3 loadcell amps and 1 e-match driver to accomodate for the rocket board needs.
    // The fs board only uses 1 loadcell.
    uint16_t n2o_loadcell;
};

// Rocket (R) Loadcell and Ignition Firing Terminal (LIFT) board structure
struct r_lift {
    struct lift_metadata meta;

    union r_loadcells {
        struct {
            uint16_t thrust_loadcell1;
            uint16_t thrust_loadcell2;
            uint16_t thrust_loadcell3;
        } test_loadcells;
        struct {
            uint16_t rocket_weight_loadcell;
            uint16_t _reserved[2];
        } launch_loadcells;
        uint16_t raw[3];
    } loadcells;

    // Feels weird to have the union with only one bit,
    // but it is consistent with the hydras and other lift structs.
    union r_ematches {
        struct {
            uint8_t main_ematch: 1;
            uint8_t _reserved: 7;
        };
        uint8_t raw;
    } ematches; 
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
void rocket_lift_init(struct rocket_lift *h);

/**
 * Read the sensors from the lift.
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
void rocket_lift_sensor_read(const int client_iface, struct rocket_lift *const h);

// TODO: implement this function
void rocket_lift_coils_read(const int client_iface, struct rocket_lift *const h);

#endif // LIFT_H_
