#ifndef SERVICES_FAKE_LORA_H
#define SERVICES_FAKE_LORA_H

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/drivers/uart.h>

#include "packets.h"
#include "services/lora.h"

bool fake_lora_setup(void);
void fake_lora_backend(void);

#endif // SERVICES_FAKE_LORA_H
