#ifndef MODBUS_THRD_H_
#define MODBUS_THRD_H_

#include <stdbool.h>
#include <stdint.h>

bool modbus_service_setup(void);
void modbus_service_start(void);

#endif
