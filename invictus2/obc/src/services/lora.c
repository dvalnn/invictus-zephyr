#include "services/lora.h"

#include "filling_sm.h"

#include "zbus_messages.h"
#include "zephyr/kernel/thread_stack.h"
#include "zephyr/logging/log.h"
#include "zephyr/toolchain.h"
#include "zephyr/kernel.h"
#include "zephyr/device.h"
#include "zephyr/zbus/zbus.h"

LOG_MODULE_REGISTER(lora_thread, LOG_LEVEL_DBG);

ZBUS_CHAN_DECLARE(lora_cmd_chan);

bool lora_service_setup(void)
{
    LOG_INF("Setting up LoRa thread...");

    return lora_setup();
}

void lora_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    LOG_INF("Starting LoRa thread...");
    return lora_backend();
}

#define LORA_THREAD_PRIO 5               // TODO: make KConfig
K_THREAD_STACK_DEFINE(lora_stack, 1024); // TODO: make KConfig
static struct k_thread lora_thread;

void lora_service_start(void)
{
    const k_tid_t tid =
        k_thread_create(&lora_thread, lora_stack, K_THREAD_STACK_SIZEOF(lora_stack),
                        lora_thread_entry, NULL, NULL, NULL, LORA_THREAD_PRIO, 0, K_NO_WAIT);

    k_thread_name_set(tid, "lora");
}
