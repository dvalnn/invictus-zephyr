#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include "zephyr/sys/atomic.h"
#include "zephyr/sys/atomic_types.h"
#include "zephyr/zbus/zbus.h"

#include <zephyr/drivers/gpio.h>

#include "data_models.h"
#include "validators.h"
#include "radio_commands.h"

#include "services/lora.h"
#include "services/modbus.h"
#include "services/rocket_state.h"

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

// FIXME: remove, it's just to make sure linker is working
#include "invictus2/drivers/sx128x_hal.h"

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
LOG_MODULE_REGISTER(obc, LOG_LEVEL_INF);

// --- ZBUS Channel Definitions ---
//
//
// --- Sensor Channels ---
ZBUS_CHAN_DEFINE(chan_thermo_sensors,   /* Channel Name */
                 union thermocouples_u, /* Message Type */
                 NULL,                  /* Validator Func */
                 NULL,                  /* User Data */
                 ZBUS_OBSERVERS_EMPTY,  /* Observers */
                 ZBUS_MSG_INIT(0)       /* Initial Value */
)

ZBUS_CHAN_DEFINE(chan_pressure_sensors, /* Channel Name */
                 union pressures_u,     /* Message Type */
                 NULL,                  /* Validator Func */
                 NULL,                  /* User Data */
                 ZBUS_OBSERVERS_EMPTY,  /* Observers */
                 ZBUS_MSG_INIT(0)       /* Initial Value */
)

ZBUS_CHAN_DEFINE(chan_weight_sensors,      /* Channel Name */
                 union loadcell_weights_u, /* Message Type */
                 NULL,                     /* Validator Func */
                 NULL,                     /* User Data */
                 ZBUS_OBSERVERS_EMPTY,     /* Observers */
                 ZBUS_MSG_INIT(0)          /* Initial Value */
)

ZBUS_CHAN_DEFINE(chan_navigator_sensors,     /* Channel Name */
                 struct navigator_sensors_s, /* Message Type */
                 NULL,                       /* Validator Func */
                 NULL,                       /* User Data */
                 ZBUS_OBSERVERS_EMPTY,       /* Observers */
                 ZBUS_MSG_INIT(0)            /* Initial Value */
)

// --- Mobdus Actuator Write Channel ---
ZBUS_CHAN_DEFINE(chan_actuators,           /* Channel Name */
                 union actuators_bitmap_u, /* Message Type */
                 NULL,                     /* Validator Func */
                 NULL,                     /* User Data */
                 ZBUS_OBSERVERS_EMPTY,     /* Observers */
                 ZBUS_MSG_INIT(0)          /* Initial Value */
)

// --- Radio Commands from Ground Station ---
ZBUS_CHAN_DEFINE(chan_radio_cmds,            /* Channel Name */
                 struct radio_generic_cmd_s, /* Message Type */
                 radio_cmd_validator,        /* Validator Func */
                 NULL,                       /* User Data */
                 ZBUS_OBSERVERS_EMPTY,       /* Observers */
                 ZBUS_MSG_INIT(0)            /* Initial Value */
)

// --- Rocket State ---
ZBUS_CHAN_DEFINE(chan_rocket_state,      /* Channel Name */
                 struct rocket_state_s,  /* Message Type */
                 rocket_state_validator, /* Validator Func */
                 NULL,                   /* User Data */
                 ZBUS_OBSERVERS_EMPTY,   /* Observers */
                 ZBUS_MSG_INIT(0)        /* Initial Value */
)

static lora_context_t lora_context;

// --- Thread Spawning ---
int ret;

bool setup_services(atomic_t *stop_signal)
{
    if (!gpio_is_ready_dt(&led)) {
        return 0;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return 0;
    }
    LOG_INF("Setting up threads...");
    lora_context.stop_signal = stop_signal;

    LOG_INF("  * rocket state...");
    if (!rocket_state_service_setup()) {
        LOG_ERR("Rocket state service setup failed");
        return false;
    }

    /* Initialize the modbus thread */
    LOG_INF("  * modbus...");
    if (!modbus_service_setup()) {
        LOG_ERR("Modbus RTU master setup failed");
        return false;
    }

    LOG_INF("  * lora...");
    if (!lora_service_setup(&lora_context)) {
        LOG_ERR("LoRa thread setup failed");
        return false;
    }
    LOG_INF("done...");
    return true;
}

// --- Main ---
int main(void)
{
    LOG_INF("Starting OBC main thread");
    atomic_t stop_signal = ATOMIC_INIT(0);

    if (!setup_services(&stop_signal)) {
        LOG_ERR("Failed to setup services");
        return -1;
    }

    rocket_state_service_start();
    /* modbus_service_start(); */
    lora_service_start();

    LOG_INF("Services started. Sleeping main thread.");

    while (1) {
        ret = gpio_pin_toggle_dt(&led);
        if (ret < 0) {
            return 0;
        }
        k_sleep(K_MSEC(1000));
    }
    k_sleep(K_FOREVER);

    k_oops(); // Should never reach here
}
