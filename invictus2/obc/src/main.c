#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include "zephyr/toolchain.h"

#include "filling_sm.h"
#include "modbus_thrd.h"
#include "lora_thrd.h"

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

LOG_MODULE_REGISTER(obc, LOG_LEVEL_INF);

// --- Thread Config ---
#define THREAD_STACK_SIZE      2048
#define THREAD_PRIORITY_LOW    K_PRIO_PREEMPT(10)
#define THREAD_PRIORITY_MEDIUM K_PRIO_PREEMPT(5)
#define THREAD_PRIORITY_HIGH   K_PRIO_PREEMPT(1)

// --- Queues ---
K_MSGQ_DEFINE(fsm_cmd_q, sizeof(cmd_t), 2, 1);
K_MSGQ_DEFINE(modbus_sensor_q, sizeof(union filling_data), 2, 1);

static const struct modbus_data_queues mb_queues = {
    .fsm_cmd_q = &fsm_cmd_q,
    .sensor_data_q = &modbus_sensor_q,
};

// --- Filling FSM Config ---
DEFAULT_FSM_CONFIG(filling_sm_config);

// --- Max Threads ---
#define N_THREADS 4

// --- Stacks ---
K_THREAD_STACK_DEFINE(modbus_stack, THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(lora_stack, THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(navigator_stack, THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(data_stack, THREAD_STACK_SIZE);

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
    {.tid = NULL, .joined = false, .name = "data"},
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

void data_thread_entry(void *modbus_msgq, void *p2, void *p3)
{
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    struct k_msgq *mbus_q = modbus_msgq;
    if (mbus_q == NULL) {
        LOG_ERR("Modbus message queue is NULL");
        return;
    }

    union filling_data modbus_data = {0};
    union filling_data modbus_batch[512 / sizeof(modbus_data)] = {0};
    int batch_size = 0;

    while (1) {
        const int ret = k_msgq_get(mbus_q, &modbus_data, K_FOREVER);
        if (ret) {
            LOG_ERR("Failed to get data from modbus message queue: %d", ret);
            return;
        }
        modbus_batch[batch_size++] = modbus_data;

        LOG_FILLING_DATA(modbus_data);
        if (batch_size >= ARRAY_SIZE(modbus_batch)) {
            // Simulate saving a batch of data to SD card
            LOG_INF("Saving batch of %d modbus data entries to SD card", batch_size);
            // Simulate saving data to SD card
            // TODO: Implement actual SD card logic
            k_sleep(K_MSEC(10)); // Simulate write delay
            batch_size = 0;      // Reset batch size after saving
        }
    }
}

// --- Thread Spawning ---

bool setup_all_threads(void)
{
    LOG_INF("Setting up threads...");

    if (!lora_thread_setup()) {
        LOG_ERR("LoRa thread setup failed");
        return false;
    }

    // Initialize the modbus thread
    if (!modbus_thread_setup()) {
        LOG_ERR("Modbus RTU master initialization failed");
        return false;
    }

    return true;
}

void spawn_all_threads(void)
{
    LOG_INF("Spawning threads dynamically...");

    threads[0].tid =
        k_thread_create(&thread_data[0], modbus_stack, THREAD_STACK_SIZE, modbus_thread_entry,
                        (void *)&filling_sm_config, (void *)&mb_queues, NULL,
                        THREAD_PRIORITY_MEDIUM, 0, K_NO_WAIT);

    threads[1].tid =
        k_thread_create(&thread_data[1], lora_stack, THREAD_STACK_SIZE, lora_thread_entry,
                        NULL, NULL, NULL, THREAD_PRIORITY_LOW, 0, K_NO_WAIT);

    threads[2].tid = k_thread_create(&thread_data[2], navigator_stack, THREAD_STACK_SIZE,
                                     navigator_thread_entry, NULL, NULL, NULL,
                                     THREAD_PRIORITY_LOW, 0, K_NO_WAIT);

    threads[3].tid =
        k_thread_create(&thread_data[3], data_stack, THREAD_STACK_SIZE, data_thread_entry,
                        &modbus_sensor_q, NULL, NULL, THREAD_PRIORITY_LOW, 0, K_NO_WAIT);

    for (int i = 0; i < N_THREADS; i++) {
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

    LOG_INF("Entering main join loop");
    join_all_threads();

    k_oops(); // Should never reach here
}
