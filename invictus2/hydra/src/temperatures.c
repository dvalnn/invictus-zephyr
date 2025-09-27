#include "temperatures.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(temperatures, LOG_LEVEL_INF);

#define THERMOS_THREAD_STACK_SIZE 1024 // TODO: make KConfig
#define THERMOS_THREAD_PRIORITY   5    // TODO: make KConfig

K_THREAD_STACK_DEFINE(thermos_thread_stack, THERMOS_THREAD_STACK_SIZE);
static struct k_thread thermos_thread_data;

int temperatures_init(void)
{
    // Initialization code for temperature sensors would go here
    LOG_INF("Temperature sensors initialized");
    return 0;
}

void thermos_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    LOG_INF("Thermocouple polling thread starting");

    while (1)
    {
        k_sleep(K_MSEC(1000));
        // Read temperatures and publish to ZBUS
    }
}

void temperatures_start(void)
{
    k_tid_t tid =
        k_thread_create(&thermos_thread_data, thermos_thread_stack,
                        K_THREAD_STACK_SIZEOF(thermos_thread_stack), thermos_thread_entry,
                        NULL, NULL, NULL, THERMOS_THREAD_PRIORITY, 0, K_NO_WAIT);

    k_thread_name_set(tid, "thermos_thread");
    LOG_INF("Temperature monitoring started");
}
