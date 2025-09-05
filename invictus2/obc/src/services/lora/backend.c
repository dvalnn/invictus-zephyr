#include "services/lora.h"
#include "zephyr/logging/log.h"
#include "zephyr/kernel.h"

LOG_MODULE_REGISTER(lora_backend, LOG_LEVEL_DBG);

bool lora_setup(lora_context_t *context)
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

