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
    struct k_sem data_available;
    // Rx buffer from lora device
    struct ring_buf rx_rb;
    // size of last read operation
    size_t rx_size;
} lora_context_t;

bool lora_service_setup(lora_context_t *);
void lora_service_start(void);

#endif // LORA_THRD_H_
