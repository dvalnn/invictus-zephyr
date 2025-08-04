#ifndef MODBUS_THRD_H_
#define MODBUS_THRD_H_

#include "stdbool.h"

bool modbus_thread_setup(void);
void modbus_thread(void *p1, void *p2, void *p3);

#endif
