#ifndef _MODBUS_THRD_H_
#define _MODBUS_THRD_H_

#include "zephyr/kernel.h"

struct modbus_data_queues {
    struct k_msgq *fsm_cmd_q;
    struct k_msgq *sensor_data_q;
};

bool modbus_thread_setup(void);
void modbus_thread_entry(void *fsm_config, void *data_queues, void *can_start_sem);

#endif
