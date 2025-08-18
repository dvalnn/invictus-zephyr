#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include "zephyr/zbus/zbus.h"

#include "filling_sm.h"
#include "zbus_messages.h"
#include "radio_commands.h"

#include "services/lora.h"
#include "services/modbus/modbus.h"
#include "services/sd_storage.h"

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

// ---
// == DEPRECATED == ZBUS Definitions == DEPRECATED == ---
// ---
ZBUS_CHAN_DEFINE(uf_hydra_chan,        /* Channel Name */
                 struct uf_hydra_msg,  /* Message Type */
                 NULL,                 /* Validator Func */
                 NULL,                 /* User Data*/
                 ZBUS_OBSERVERS_EMPTY, /* Observers */
                 ZBUS_MSG_INIT(.uf_temperature1 = 0, .uf_temperature2 = 0,
                               .uf_temperature3 = 0) /* Initial Value */
);

// DEPRECATED
ZBUS_CHAN_DEFINE(lf_hydra_chan,        /* Channel Name */
                 struct lf_hydra_msg,  /* Message Type */
                 NULL,                 /* Validator Func */
                 NULL,                 /* User Data*/
                 ZBUS_OBSERVERS_EMPTY, /* Observers */
                 ZBUS_MSG_INIT(.lf_temperature1 = 0, .lf_temperature2 = 0, .lf_pressure = 0,
                               .cc_pressure = 0)
                 /* Initial Value */
);

// DEPRECATED
ZBUS_CHAN_DEFINE(fs_hydra_chan,        /* Channel Name */
                 struct fs_hydra_msg,  /* Message Type */
                 NULL,                 /* Validator Func */
                 NULL,                 /* User Data*/
                 ZBUS_OBSERVERS_EMPTY, /* Observers */
                 ZBUS_MSG_INIT()       /* Initial Value */
);

// DEPRECATED
ZBUS_CHAN_DEFINE(r_lift_chan,          /* Channel Name */
                 struct r_lift_msg,    /* Message Type */
                 NULL,                 /* Validator Func */
                 NULL,                 /* User Data*/
                 ZBUS_OBSERVERS_EMPTY, /* Observers */
                 ZBUS_MSG_INIT(.loadcell1 = 0, .loadcell2 = 0, .loadcell3 = 0,
                               .main_ematch = 0) /* Initial Value */
);

// DEPRECATED
ZBUS_CHAN_DEFINE(fs_lift_chan,                    /* Channel Name */
                 struct fs_lift_msg,              /* Message Type */
                 NULL,                            /* Validator Func */
                 NULL,                            /* User Data*/
                 ZBUS_OBSERVERS_EMPTY,            /* Observers */
                 ZBUS_MSG_INIT(.n2o_loadcell = 0) /* Initial Value */
);

// DEPRECATED
ZBUS_CHAN_DEFINE(modbus_write_coils_chan,          /* Channel Name */
                 struct modbus_write_coils_msg,    /* Message Type */
                 modbus_write_coils_msg_validator, /* Validator Func */
                 NULL,                             /* User Data*/
                 ZBUS_OBSERVERS_EMPTY,             /* Observers */
                 ZBUS_MSG_INIT(.slave_id = 0, .start_addr = 0, .values = NULL,
                               .num_coils = 0) /* Initial Value */

);

// DEPRECATED
ZBUS_CHAN_DEFINE(rocket_event_chan,                        /* Channel Name */
                 struct rocket_event_msg,                  /* Message Type */
                 rocket_event_msg_validator,               /* Validator Func */
                 NULL,                                     /* User Data*/
                 ZBUS_OBSERVERS_EMPTY,                     /* Observers */
                 ZBUS_MSG_INIT(.event = ROCKET_EVENT_NONE) /* Initial Value */
);

// DEPRECATED
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
    sd_service_start();

    LOG_INF("Work queues started. Sleeping main thread.");
    k_sleep(K_FOREVER);

    k_oops(); // Should never reach here
}
