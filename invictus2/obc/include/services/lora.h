#ifndef LORA_THRD_H_
#define LORA_THRD_H_

#include "stdbool.h"
#include <assert.h>
#include "zephyr/kernel.h"
#include "zephyr/sys/atomic_types.h"

// Set to use production backend
#define CONFIG_LORA_BACKEND       1
// Set to use backend for testing
#define CONFIG_LORA_REDIRECT_UART 0

typedef struct lora_context {
    atomic_t *stop_signal;
    // Signal from lora device
    struct k_sem sem_data_available;
    // Rx buffer from lora device
    struct ring_buf rx_rb;
} lora_context_t;

bool lora_setup(void);
void lora_backend(void);

bool lora_service_setup(void);
void lora_service_start(atomic_t *stop_signal);

#endif // LORA_THRD_H_
