#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>

#include <zephyr/sys/atomic.h>
#include <zephyr/sys/atomic_types.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>

#if DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_shell_uart), zephyr_cdc_acm_uart)
#include <zephyr/drivers/uart.h>
#endif // zephyr_shell_uart

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(obc, LOG_LEVEL_INF);

#include "data_models.h"
#include "validators.h"
#include "packets.h"
#include "shell.h"

#include "services/lora.h"
#include "services/modbus.h"
#include "services/state_machine/main_sm.h"

static const struct gpio_dt_spec led_green =
    GPIO_DT_SPEC_GET(DT_NODELABEL(led_green_0), gpios);
static const struct gpio_dt_spec led_red = GPIO_DT_SPEC_GET(DT_NODELABEL(led_red_0), gpios);
static const struct pwm_dt_spec buzzer = PWM_DT_SPEC_GET(DT_NODELABEL(buzzer));

#define BUZZER_PULSE_MICROS 500

/* static const struct device *fem = DEVICE_DT_GET(DT_NODELABEL(nrf_radio_fem)); */

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

// --- ZBUS Channel Definitions ---
//
//
// --- Sensor Channels ---
ZBUS_CHAN_DEFINE(chan_thermo_sensors,  /* Channel Name */
                 thermocouples_t,      /* Message Type */
                 NULL,                 /* Validator Func */
                 NULL,                 /* User Data */
                 ZBUS_OBSERVERS_EMPTY, /* Observers */
                 ZBUS_MSG_INIT(0)      /* Initial Value */
);

ZBUS_CHAN_DEFINE(chan_pressure_sensors, /* Channel Name */
                 pressures_t,           /* Message Type */
                 NULL,                  /* Validator Func */
                 NULL,                  /* User Data */
                 ZBUS_OBSERVERS_EMPTY,  /* Observers */
                 ZBUS_MSG_INIT(0)       /* Initial Value */
);

ZBUS_CHAN_DEFINE(chan_weight_sensors,  /* Channel Name */
                 loadcell_weights_t,   /* Message Type */
                 NULL,                 /* Validator Func */
                 NULL,                 /* User Data */
                 ZBUS_OBSERVERS_EMPTY, /* Observers */
                 ZBUS_MSG_INIT(0)      /* Initial Value */
);

ZBUS_CHAN_DEFINE(chan_navigator_sensors, /* Channel Name */
                 navigator_sensors_t,    /* Message Type */
                 NULL,                   /* Validator Func */
                 NULL,                   /* User Data */
                 ZBUS_OBSERVERS_EMPTY,   /* Observers */
                 ZBUS_MSG_INIT(0)        /* Initial Value */
);

ZBUS_CHAN_DEFINE(chan_kalman_data,     /* Channel Name */
                 kalman_data_t,        /* Message Type */
                 NULL,                 /* Validator Func */
                 NULL,                 /* User Data */
                 ZBUS_OBSERVERS_EMPTY, /* Observers */
                 ZBUS_MSG_INIT(0)      /* Initial Value */
);

// --- Mobdus Actuator Write Channel ---
ZBUS_CHAN_DEFINE(chan_actuators,       /* Channel Name */
                 actuators_bitmap_t,   /* Message Type */
                 NULL,                 /* Validator Func */
                 NULL,                 /* User Data */
                 ZBUS_OBSERVERS_EMPTY, /* Observers */
                 ZBUS_MSG_INIT(0)      /* Initial Value */
);

// --- Packets from Ground Station ---
ZBUS_CHAN_DEFINE(chan_packets,     /* Channel Name */
                 generic_packet_t, /* Message Type */
                 NULL,             /* Validator Func */
                 /* packet_validator,     /\* Validator Func *\/ */
                 NULL,                 /* User Data */
                 ZBUS_OBSERVERS_EMPTY, /* Observers */
                 ZBUS_MSG_INIT(0)      /* Initial Value */
);

// --- Rocket State ---
ZBUS_CHAN_DEFINE(chan_rocket_state,    /* Channel Name */
                 state_data_t,         /* Message Type */
                 NULL,                 /* Validator Func */
                 NULL,                 /* User Data */
                 ZBUS_OBSERVERS_EMPTY, /* Observers */
                 ZBUS_MSG_INIT(0)      /* Initial Value */
);

static lora_context_t lora_context;

// --- Thread Spawning ---

bool setup_services(atomic_t *stop_signal)
{
    LOG_INF("Setting up threads...");
    lora_context.stop_signal = stop_signal;

    LOG_INF("  * state machine...");
    if (!state_machine_service_setup())
    {
        LOG_ERR("State machine service setup failed");
        return false;
    }

    // Initialize the modbus thread
    LOG_INF("  * modbus...");
    if (!modbus_service_setup())
    {
        LOG_ERR("Modbus RTU master setup failed");
        return false;
    }

    /* LOG_INF("  * lora..."); */
    /* if (!lora_service_setup(&lora_context)) */
    /* { */
    /*     LOG_ERR("LoRa thread setup failed"); */
    /*     return false; */
    /* } */

    LOG_INF("done...");
    return true;
}

void health_check(void)
{
    LOG_INF("Heartbeat");

    k_sleep(K_MSEC(1000));
    pwm_set_pulse_dt(&buzzer, BUZZER_PULSE_MICROS * 1000);
    k_sleep(K_MSEC(1000));
    pwm_set_pulse_dt(&buzzer, 0);
}

// --- Main ---
int main(void)
{
    LOG_INF("Starting OBC main thread");

    health_check();
    setup_shell_if_enabled();
    health_check();

    LOG_INF("Setting up services");
    atomic_t stop_signal = ATOMIC_INIT(0);
    if (!setup_services(&stop_signal))
    {
        LOG_ERR("Failed to setup services");
        k_oops();
    }

    /* lora_service_start(); */
    state_machine_service_start();
    // modbus_service_start();

    LOG_INF("Services started.");
    return 0;
}
