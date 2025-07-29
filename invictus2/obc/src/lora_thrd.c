#include "lora_thrd.h"

#if CONFIG_LORA_REDIRECT_UART

// defer to lora_uart_shell.c for implementation
#include "lora_uart_shell.h"

bool lora_thread_setup(void)
{
    return lora_uart_setup();
}

void lora_thread_entry(void *p1, void *p2, void *p3)
{
    return lora_uart_entry(p1, p2, p3);
}

#else

#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include "zephyr/toolchain.h"

LOG_MODULE_REGISTER(lora_thread, LOG_LEVEL_DBG);

bool lora_thread_setup(void)
{
    // TODO
    return true;
}

void lora_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    // TODO

    LOG_INF("LoRa thread running mock logic...");
    k_sleep(K_SECONDS(3)); // Simulate work
    LOG_INF("LoRa thread exiting.");
}

#endif
