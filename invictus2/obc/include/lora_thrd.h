#ifndef LORA_THRD_H_
#define LORA_THRD_H_

#include "stdbool.h"

bool lora_thread_setup(void);
void lora_thread_entry(void *p1, void *p2, void *p3);

#endif // LORA_THRD_H_
