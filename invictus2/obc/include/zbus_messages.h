#ifndef ZBUS_H_
#define ZBUS_H_

#include "stdint.h"
#include "stdbool.h"
#include "zephyr/kernel.h"

struct uf_hydra_msg {     // upper feed hydra
    uint16_t temperature; // Upper feed temperature probe temperature in ºC
};

struct lf_hydra_msg {        // lower feed hydra
    uint16_t lf_temperature; // Lower feed temperature probe temperature in ºC
    uint16_t lf_pressure;    // Lower feed pressure in bar
    uint16_t cc_pressure;    // Combustion chamber pressure in bar
};

struct fs_hydra_msg { // filling station hydra
    // TODO: Add filling station hydra message structure
};

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

bool modbus_coil_write_msg_validator(const void *msg, size_t msg_size);
bool rocket_event_msg_validator(const void *msg, size_t msg_size);

#endif // ZBUS_H_
