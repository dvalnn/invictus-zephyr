#ifndef LORA_UART_SHELL_H_
#define LORA_UART_SHELL_H_

#include "stdbool.h"

bool lora_uart_setup(void);
void lora_uart_entry(void *p1, void *p2, void *p3);

#endif // LORA_UART_SHELL_H_
