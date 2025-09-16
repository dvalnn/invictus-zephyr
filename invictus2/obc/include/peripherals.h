#ifndef PERIPHERALS_H
#define PERIPHERALS_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <inttypes.h>

void pwm_init(void);

#endif // PERIPHERALS_H
