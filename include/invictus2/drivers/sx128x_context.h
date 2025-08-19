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
    enum sx128x_sleep_status sleep_status;
    uint8_t tx_offset;
};

void sx1280x_write(uint8_t *, size_t);
void sx128x_register_recv_callback(void (*rx_callback)(uint8_t *, uint16_t));

#endif // SX128X_HAL_CONTEXT_H
