#ifndef MODBUS_THRD_H_
#define MODBUS_THRD_H_

#include "stdbool.h"

// TODO: make this Kconfig
#ifndef MODBUS_LISTENER_PRIO
#define MODBUS_LISTENER_PRIO 5
#endif

#ifndef MODBUS_WORKER_PRIO
#define MODBUS_WORKER_PRIO 5
#endif

bool modbus_service_setup(void);
void modbus_service_start(void);

#endif
