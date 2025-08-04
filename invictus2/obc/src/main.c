#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include "zephyr/zbus/zbus.h"

#include "filling_sm.h"
#include "zbus_messages.h"

#include "services/modbus.h"
#include "services/sd_storage.h"

LOG_MODULE_REGISTER(obc, LOG_LEVEL_INF);

// THREADS:
// - Main thread: LoRa communication. (for now just pipe everything to stdout).
//   - Pre-Flight Mode: Switches between receive and transmit modes based
//   periodically -- High priority thread.
//   - Flight Model: passively sends data to the ground station. Should not
//   block other threads, switch to low priority.
// - Modbus thread: Reads holding registers from the slave devices and updates
// the filling state machine. -- Medium priority thread.
// - Navigator thread: Reads data from the navigator board via uart. -- Medium
// priority thread.
// - Data Thread: Saves sensor data and debug logs to the SD card. -- Low
// priority thread.
//
// LOGGING:
// - Log debugg information to the console and to a file on the SD card.
// // - Use LOG_INF, LOG_ERR, LOG_DBG, etc. macros for logging.

// --- ZBUS Definitions ---

ZBUS_CHAN_DEFINE(uf_hydra_chan,                  /* Channel Name */
                 struct uf_hydra_msg,            /* Message Type */
                 NULL,                           /* Validator Func */
                 NULL,                           /* User Data*/
                 ZBUS_OBSERVERS_EMPTY,           /* Observers */
                 ZBUS_MSG_INIT(.temperature = 0) /* Initial Value */
);

ZBUS_CHAN_DEFINE(lf_hydra_chan,        /* Channel Name */
                 struct lf_hydra_msg,  /* Message Type */
                 NULL,                 /* Validator Func */
                 NULL,                 /* User Data*/
                 ZBUS_OBSERVERS_EMPTY, /* Observers */
                 ZBUS_MSG_INIT(.lf_temperature = 0, .lf_pressure = 0, .cc_pressure = 0)
                 /* Initial Value */
);

ZBUS_CHAN_DEFINE(fs_hydra_chan,        /* Channel Name */
                 struct fs_hydra_msg,  /* Message Type */
                 NULL,                 /* Validator Func */
                 NULL,                 /* User Data*/
                 ZBUS_OBSERVERS_EMPTY, /* Observers */
                 ZBUS_MSG_INIT()       /* Initial Value */
);

ZBUS_CHAN_DEFINE(modbus_coil_write_chan,          /* Channel Name */
                 struct modbus_write_coils_msg,   /* Message Type */
                 modbus_coil_write_msg_validator, /* Validator Func */
                 NULL,                            /* User Data*/
                 ZBUS_OBSERVERS_EMPTY,            /* Observers */
                 ZBUS_MSG_INIT(.slave_id = 0, .start_addr = 0, .values = NULL,
                               .num_coils = 0) /* Initial Value */

);

ZBUS_CHAN_DEFINE(rocket_event_chan,                        /* Channel Name */
                 struct rocket_event_msg,                  /* Message Type */
                 rocket_event_msg_validator,               /* Validator Func */
                 NULL,                                     /* User Data*/
                 ZBUS_OBSERVERS_EMPTY,                     /* Observers */
                 ZBUS_MSG_INIT(.event = ROCKET_EVENT_NONE) /* Initial Value */
);

ZBUS_CHAN_DEFINE(lora_cmd_chan,        /* Channel Name */
                 struct lora_cmd_msg,  /* Message Type */
                 NULL,                 /* Validator Func */
                 NULL,                 /* User Data*/
                 ZBUS_OBSERVERS_EMPTY, /* Observers */
                 ZBUS_MSG_INIT(.subsystem = SUBSYSTEM_ID_NONE,
                               .command = 0) /* Initial Value */
);

// --- Filling FSM Config ---
DEFAULT_FSM_CONFIG(filling_sm_config);

// --- Thread Spawning ---

bool setup_services(void)
{
    LOG_INF("Setting up threads...");

    // Initialize the modbus thread
    if (!modbus_service_setup()) {
        LOG_ERR("Modbus RTU master initialization failed");
        return false;
    }

    /* if (!lora_thread_setup()) { */
    /*     LOG_ERR("LoRa thread setup failed"); */
    /*     return false; */
    /* } */

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
    start_sd_worker_q();

    LOG_INF("Work queues started. Sleeping main thread.");
    k_sleep(K_FOREVER);

    k_oops(); // Should never reach here
}
