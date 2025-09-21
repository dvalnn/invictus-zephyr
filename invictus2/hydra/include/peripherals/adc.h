#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include <zephyr/kernel.h>

void init_adcs();
int read_adcs();

#endif /* ADC_DRIVER_H */
