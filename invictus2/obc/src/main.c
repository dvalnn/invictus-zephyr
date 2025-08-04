#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include "zephyr/toolchain.h"
#include "zephyr/zbus/zbus.h"

#include "filling_sm.h"
#include "zbus_messages.h"

#include "threads/lora.h"
#include "threads/modbus.h"
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
                 ZBUS_OBSERVERS(uf_hydra_obs),   /* Observers */
                 ZBUS_MSG_INIT(.temperature = 0) /* Initial Value */
);

ZBUS_CHAN_DEFINE(lf_hydra_chan,                /* Channel Name */
                 struct lf_hydra_msg,          /* Message Type */
                 NULL,                         /* Validator Func */
                 NULL,                         /* User Data*/
                 ZBUS_OBSERVERS(lf_hydra_obs), /* Observers */
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

ZBUS_CHAN_DEFINE(modbus_coil_write_chan,                /* Channel Name */
                 struct modbus_write_coils_msg,         /* Message Type */
                 modbus_coil_write_msg_validator,       /* Validator Func */
                 NULL,                                  /* User Data*/
                 ZBUS_OBSERVERS(modbus_coil_write_obs), /* Observers */
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

ZBUS_SUBSCRIBER_DEFINE(modbus_coil_write_obs, 2);
ZBUS_SUBSCRIBER_DEFINE(uf_hydra_obs, 2);
ZBUS_SUBSCRIBER_DEFINE(lf_hydra_obs, 2);

// --- Filling FSM Config ---
DEFAULT_FSM_CONFIG(filling_sm_config);

// --- Thread Config ---
#define THREAD_STACK_SIZE      2048
#define THREAD_PRIORITY_LOW    K_PRIO_PREEMPT(10)
#define THREAD_PRIORITY_MEDIUM K_PRIO_PREEMPT(5)
#define THREAD_PRIORITY_HIGH   K_PRIO_PREEMPT(1)

// --- Max Threads ---
#define N_THREADS 3

// --- Stacks ---
K_THREAD_STACK_DEFINE(modbus_stack, THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(lora_stack, THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(navigator_stack, THREAD_STACK_SIZE);

// --- Thread structures and tracking ---
static struct k_thread thread_data[N_THREADS];

static struct {
    k_tid_t tid;
    bool joined;
    const char *name;
} threads[N_THREADS] = {
    {.tid = NULL, .joined = false, .name = "modbus"},
    {.tid = NULL, .joined = false, .name = "lora"},
    {.tid = NULL, .joined = false, .name = "navigator"},
};

void navigator_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    LOG_INF("Navigator thread running mock logic...");
    k_sleep(K_SECONDS(3)); // Simulate work
    LOG_INF("Navigator thread exiting.");
}

// --- Thread Spawning ---

bool setup_all_threads(void)
{
    LOG_INF("Setting up threads...");

    // Initialize the modbus thread
    if (!modbus_thread_setup()) {
        LOG_ERR("Modbus RTU master initialization failed");
        return false;
    }

    if (!lora_thread_setup()) {
        LOG_ERR("LoRa thread setup failed");
        return false;
    }

    return true;
}

void spawn_all_threads(void)
{
    LOG_INF("Spawning threads dynamically...");

    threads[0].tid =
        k_thread_create(&thread_data[0], modbus_stack, THREAD_STACK_SIZE, modbus_thread, NULL,
                        NULL, NULL, THREAD_PRIORITY_MEDIUM, 0, K_NO_WAIT);

    /* threads[1].tid = k_thread_create( */
    /*     &thread_data[1], lora_stack, THREAD_STACK_SIZE, lora_thread_entry, */
    /*     (void *)&lora_cmd_qs, (void *)&lora_data_qs, NULL, THREAD_PRIORITY_LOW, 0, */
    /* K_NO_WAIT); */

    /* threads[2].tid = k_thread_create(&thread_data[2], navigator_stack, THREAD_STACK_SIZE, */
    /*                                  navigator_thread_entry, NULL, NULL, NULL, */
    /*                                  THREAD_PRIORITY_LOW, 0, K_NO_WAIT); */

    for (int i = 0; i < N_THREADS; i++) {
        if (threads[i].tid == NULL) {
            continue;
        }
        k_thread_name_set(threads[i].tid, threads[i].name);
    }
}

// --- Thread Joining ---
bool handle_join_thread(k_tid_t thread, const char *name)
{
    if (thread == NULL) {
        LOG_WRN("Thread %s handle is NULL", name);
        return true; // Treat as joined
    }

    switch (k_thread_join(thread, K_NO_WAIT)) {
    case 0:
        LOG_WRN("Thread %s joined", name);
        return true;

    case -EBUSY:
        return false;

    default:
        LOG_ERR("Failed to join thread %s: %d", name, -errno);
        return true; // Treat as a joined thread for simplicity
    }
}

void join_all_threads(void)
{
    int remaining = N_THREADS;
    while (remaining > 0) {
        for (int i = 0; i < N_THREADS; i++) {
            if (threads[i].joined) {
                continue;
            }

            if (handle_join_thread(threads[i].tid, threads[i].name)) {
                threads[i].joined = true;
                remaining--;

                // TODO: handle failure/degraded logic here
            }
        }

        k_sleep(K_MSEC(100));
    }

    LOG_INF("All threads have exited.");
}

// --- Main ---
int main(void)
{
    LOG_INF("Starting OBC main thread");

    if (!setup_all_threads()) {
        LOG_ERR("Failed to setup threads");
        return -1;
    }

    spawn_all_threads();
    start_sd_worker_q();

    LOG_INF("Entering main join loop");
    join_all_threads();

    k_oops(); // Should never reach here
}
