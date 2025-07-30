#ifndef LORA_THRD_H_
#define LORA_THRD_H_

#include "stdbool.h"
#include "zephyr/kernel.h"

struct lora_cmd_queues {
    struct k_msgq *in_fsm_cmd_q;      // Queue for incomming FSM commands
    struct k_msgq *in_override_cmd_q; // Queue for incomming override/manual mode commands
};

struct lora_data_queues {
    struct k_msgq *sensor_data_q;    // Queue for sensor data (modbus)
    struct k_msgq *navigator_data_q; // Queue for navigator data
};

bool lora_thread_setup(void);
void lora_thread_entry(void *_cmd_qs, void *_data_qs, void *p3);

#endif // LORA_THRD_H_
