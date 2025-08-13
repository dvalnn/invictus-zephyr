#include "services/lora.h"

#include "zephyr/kernel/thread_stack.h"
#include "zephyr/logging/log.h"
#include "zephyr/sys/ring_buffer.h"
#include "zephyr/toolchain.h"
#include "zephyr/kernel.h"
#include "zephyr/zbus/zbus.h"
#include <stdint.h>

LOG_MODULE_REGISTER(lora_thread, LOG_LEVEL_DBG);
ZBUS_CHAN_DECLARE(lora_cmd_chan);

bool lora_service_setup(void)
{
    LOG_INF("Setting up LoRa thread...");

    return lora_setup();
}

void lora_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    if (p1 == NULL) {
        LOG_ERR("missing arguments");
        return;
    }

    // TODO this buffer should come from device driver
    // TODO fine tune size
    uint8_t lora_rx_buffer[256 * 5];

    // TODO inject this context for better testability
    lora_context_t context;
    context.stop_signal = (atomic_t *)p1;
    k_sem_init(&context.sem_data_available, 0, 1);
    ring_buf_init(&context.rx_rb, sizeof(lora_rx_buffer), lora_rx_buffer);

    LOG_INF("LoRa thread starting");
    const uint32_t c_sleep_time_ms = 80;
    // TODO optimize this away with claim API from ring_buf
    uint8_t bfr_local[256];
    while (*context.stop_signal != 1) {
        if (k_sem_take(&context.sem_data_available, K_MSEC(c_sleep_time_ms))) {
            LOG_DBG("LoRa timeout");
            continue;
        }

        uint32_t rx_size = ring_buf_get(&context.rx_rb, bfr_local, sizeof(bfr_local));
        LOG_INF("read %u bytes", rx_size);

        LOG_INF("LoRa thread");
        // TODO sleep only difference
        k_sleep(K_MSEC(c_sleep_time_ms));
    }

    LOG_INF("LoRa thread exiting.");
}

#define LORA_THREAD_PRIO 5               // TODO: make KConfig
K_THREAD_STACK_DEFINE(lora_stack, 1024); // TODO: make KConfig
static struct k_thread lora_thread;

void lora_service_start(atomic_t *stop_signal)
{
    const k_tid_t tid = k_thread_create(
        &lora_thread, lora_stack, K_THREAD_STACK_SIZEOF(lora_stack), lora_thread_entry,
        (void *)stop_signal, NULL, NULL, LORA_THREAD_PRIO, 0, K_NO_WAIT);

    k_thread_name_set(tid, "lora");
}
