#ifndef SX128X_HAL_CONTEXT_H
#define SX128X_HAL_CONTEXT_H

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

struct sx128x_context_cfg {
    struct spi_dt_spec spi;

    struct gpio_dt_spec reset;
    struct gpio_dt_spec busy;

    struct gpio_dt_spec dio1;
    struct gpio_dt_spec dio2;
    struct gpio_dt_spec dio3;
};

enum sx128x_sleep_status {
    SX128X_SLEEP,
    SX128X_AWAKE,
};

struct sx128x_context_data {
    // const struct device *sx128x_dev;

    enum sx128x_sleep_status sleep_status;
    uint8_t tx_offset;
};

#endif // SX128X_HAL_CONTEXT_H
