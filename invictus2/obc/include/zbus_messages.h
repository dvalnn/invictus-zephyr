#ifndef ZBUS_H_
#define ZBUS_H_

// REVIEW: Thinking about deprecating this entire file in favor of a more
// organic approach to ZBUS messages, where each module defines its own messages
// and validators, and the ZBUS channel is defined in the module's source file.

#include "stdint.h"
#include "stdbool.h"
#include "stddef.h"

struct uf_hydra_msg {     // upper feed hydra
    uint16_t uf_temperature1; // Upper feed temperature probe 1 in ºC
    uint16_t uf_temperature2; // Upper feed temperature probe 2 in ºC
    uint16_t uf_temperature3; // Upper feed temperature probe 3 in ºC
};

struct lf_hydra_msg {        // lower feed hydra
    uint16_t lf_temperature1; // Lower feed temperature probe 1 in ºC
    uint16_t lf_temperature2; // Lower feed temperature probe 2 in ºC
    uint16_t lf_pressure;      // Lower feed pressure in bar
    uint16_t cc_pressure;      // Combustion chamber pressure in bar
};

struct fs_hydra_msg { // filling station hydra
    uint16_t n2o_pressure; // N2O pressure in bar
    uint16_t n2_pressure;  // N2 pressure in bar
    uint16_t quick_dc_pressure; // Quick disconnect pressure in bar
    uint16_t n2o_temperature1;   // N2O temperature before solenoid in ºC
    uint16_t n2o_temperature2;   // N2O temperature after solenoid in ºC
    uint16_t n2_temperature;      // N2 temperature in ºC
};

struct r_lift_msg {
    uint16_t loadcell1;  // loadcell 1 value
    uint16_t loadcell2;  // loadcell 2 value
    uint16_t loadcell3;  // loadcell 3 value
    uint8_t main_ematch; // Main e-match state (0 or 1)
};

struct fs_lift_msg {
    uint16_t n2o_loadcell; // N2O loadcell value
};

// REVIEW: thiking about deprecating this in favor of some extra helper functions
struct modbus_write_coils_msg {
    const uint16_t slave_id;
    const uint16_t start_addr; // Coil address to write to
    uint8_t *values;           // Pointer to an array of coil values (0 or 1)
    const uint16_t num_coils;  // Number of coils to write
};

enum rocket_event {
    ROCKET_EVENT_NONE = 0,
    ROCKET_EVENT_FILLING_START,
    ROCKET_EVENT_FILLING_ABORT, // Maybe not necessary
    ROCKET_EVENT_FILLING_FINISH,
    ROCKET_EVENT_LAUNCH,

    // Metadata
    _ROCKET_EVENT_COUNT,
};

struct rocket_event_msg {
    enum rocket_event event;
};

enum subsystem_id {
    SUBSYSTEM_ID_NONE = 0,
    SUBSYSTEM_ID_MODBUS,
    SUBSYSTEM_ID_LORA,
    SUBSYSTEM_ID_NAVIGATOR,
    SUBSYSTEM_ID_DATA,
    SUBSYSTEM_ID_FSM, // Filling State Machine

    // Metadata
    _SUBSYSTEM_ID_COUNT,
};

struct lora_cmd_msg {
    enum subsystem_id subsystem; // Subsystem ID for the command
    uint32_t command;
};

/* ----------------------------------------------------------- */

bool modbus_write_coils_msg_validator(const void *msg, size_t msg_size);
bool rocket_event_msg_validator(const void *msg, size_t msg_size);

#endif // ZBUS_H_
