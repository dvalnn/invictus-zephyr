#include "services/lora.h"

#if CONFIG_LORA_BACKEND

#include "zephyr/logging/log.h"
#include "zephyr/kernel.h"

LOG_MODULE_REGISTER(lora_backend, LOG_LEVEL_DBG);

bool lora_setup(void)
{
  return true;
}

void lora_backend(void)
{
    // TODO
    LOG_INF("LoRa thread starting");
    while(1)
    {
      LOG_INF("LoRa thread");
      k_sleep(K_SECONDS(3)); // Simulate work
    }

    LOG_INF("LoRa thread exiting.");
}

#endif /* ifdef CONFIG_LORA_BACKEND */
