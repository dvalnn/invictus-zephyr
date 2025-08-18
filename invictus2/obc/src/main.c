#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include "zephyr/zbus/zbus.h"

#include "filling_sm.h"
#include "radio_commands.h"

#include "services/lora.h"
#include "services/modbus/modbus.h"

LOG_MODULE_REGISTER(obc, LOG_LEVEL_INF);

// --- ZBUS Channel Definitions ---
ZBUS_CHAN_DEFINE(chan_thermo_sensors,      /* Channel Name */
                 union thermocouples_u,    /* Message Type */
                 NULL,                     /* Validator Func */
                 NULL,                     /* User Data */
                 ZBUS_OBSERVERS_EMPTY,     /* Observers */
                 ZBUS_MSG_INIT(.raw = {0}) /* Initial Value */
)

ZBUS_CHAN_DEFINE(chan_pressure_sensors,    /* Channel Name */
                 union pressures_u,        /* Message Type */
                 NULL,                     /* Validator Func */
                 NULL,                     /* User Data */
                 ZBUS_OBSERVERS_EMPTY,     /* Observers */
                 ZBUS_MSG_INIT(.raw = {0}) /* Initial Value */
)

ZBUS_CHAN_DEFINE(chan_weight_sensors,      /* Channel Name */
                 union loadcell_weights_u, /* Message Type */
                 NULL,                     /* Validator Func */
                 NULL,                     /* User Data */
                 ZBUS_OBSERVERS_EMPTY,     /* Observers */
                 ZBUS_MSG_INIT(.raw = {0}) /* Initial Value */
)

ZBUS_CHAN_DEFINE(chan_actuators,           /* Channel Name */
                 union actuators_bitmap_u, /* Message Type */
                 NULL,                     /* Validator Func */
                 NULL,                     /* User Data */
                 ZBUS_OBSERVERS_EMPTY,     /* Observers */
                 ZBUS_MSG_INIT(.raw = 0)   /* Initial Value */
);

ZBUS_CHAN_DEFINE(chan_navigator_sensors,     /* Channel Name */
                 struct navigator_sensors_s, /* Message Type */
                 NULL,                       /* Validator Func */
                 NULL,                       /* User Data */
                 ZBUS_OBSERVERS_EMPTY,       /* Observers */
                 ZBUS_MSG_INIT(0)            /* Initial Value */
);

ZBUS_CHAN_DEFINE(chan_radio_cmds,            /* Channel Name */
                 struct radio_generic_cmd_s, /* Message Type */
                 NULL,                       /* Validator Func */
                 NULL,                       /* User Data */
                 ZBUS_OBSERVERS_EMPTY,       /* Observers */
                 ZBUS_MSG_INIT(0)            /* Initial Value */
)

// --- Filling FSM Config ---
DEFAULT_FSM_CONFIG(filling_sm_config);

// --- Service Setup ---
bool setup_services(void)
{
    LOG_INF("Setting up threads...");

    // Initialize the modbus thread
    if (!modbus_service_setup()) {
        LOG_ERR("Modbus RTU master initialization failed");
        return false;
    }

    if (!lora_service_setup()) {
        LOG_ERR("LoRa thread setup failed");
        return false;
    }

    return true;
}

// --- Main ---
int main(void)
{
    LOG_INF("Starting OBC main thread");

    if (!setup_services()) {
        LOG_ERR("Failed to setup services");
        return -1;
    }

    modbus_service_start();
    lora_service_start();

    LOG_INF("Work queues started. Sleeping main thread.");
    k_sleep(K_FOREVER);

    k_oops(); // Should never reach here
}
