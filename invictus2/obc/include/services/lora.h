#ifndef LORA_THRD_H_
#define LORA_THRD_H_

#include "stdbool.h"
#include <assert.h>

// Set to use production backend
#define CONFIG_LORA_BACKEND 1
// Set to use backend for testing
#define CONFIG_LORA_REDIRECT_UART 0

bool lora_setup(void);
void lora_backend(void);

bool lora_service_setup(void);
void lora_service_start(void);

#endif // LORA_THRD_H_
