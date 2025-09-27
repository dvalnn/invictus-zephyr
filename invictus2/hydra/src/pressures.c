#include "pressures.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pressures, LOG_LEVEL_INF);

#define PRESSURES_THREAD_STACK_SIZE 1024 // TODO: make KConfig
#define PRESSURES_THREAD_PRIORITY   5    // TODO: make KConfig

K_THREAD_STACK_DEFINE(pressures_thread_stack, PRESSURES_THREAD_STACK_SIZE);
static struct k_thread pressures_thread_data;

int pressures_init(void)
{
    // Initialization code for pressure sensors would go here
    LOG_INF("Pressure sensors initialized");
    return 0;
}

void pressures_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    LOG_INF("Pressure polling thread starting");

    while (1)
    {
        k_sleep(K_MSEC(1000));
        // Read pressures and publish to ZBUS
    }
}

void pressures_start(void)
{
    k_tid_t tid =
        k_thread_create(&pressures_thread_data, pressures_thread_stack,
                        K_THREAD_STACK_SIZEOF(pressures_thread_stack), pressures_thread_entry,
                        NULL, NULL, NULL, PRESSURES_THREAD_PRIORITY, 0, K_NO_WAIT);

    k_thread_name_set(tid, "pressures_thread");
    LOG_INF("Pressure monitoring started");
}

int mv_to_mbar(int mv, int32_t min_bar, int32_t max_bar)
{
    return (((mv * max_bar) / 10) + min_bar);
}
