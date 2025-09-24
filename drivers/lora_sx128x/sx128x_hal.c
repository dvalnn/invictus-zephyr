#include <invictus2/drivers/sx128x_hal.h>
#include <invictus2/drivers/sx128x_context.h>

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sx128x_hal, CONFIG_LORA_SX128X_LOG_LEVEL);

#define PARSE_AND_VALIDATE_CTX(ctx)                                                           \
    const struct device *dev = (const struct device *)ctx;                                    \
    if (dev == NULL) {                                                                        \
        LOG_ERR("Invalid context");                                                           \
        return SX128X_HAL_STATUS_ERROR;                                                       \
    }                                                                                         \
    const struct sx128x_context_cfg *config = dev->config;                                    \
    if (config == NULL) {                                                                     \
        LOG_ERR("Invalid context config");                                                    \
        return SX128X_HAL_STATUS_ERROR;                                                       \
    }                                                                                         \
    struct sx128x_context_data *dev_data = dev->data;                                         \
    if (dev_data == NULL) {                                                                   \
        LOG_ERR("Invalid context data");                                                      \
        return SX128X_HAL_STATUS_ERROR;                                                       \
    }

static void sx128x_hal_wait_on_busy(const struct sx128x_context_cfg *config)
{
    // bool ret =
    //     WAIT_FOR(gpio_pin_get_dt(&config->busy) == 0,
    //              (1000 * CONFIG_LORA_SX128X_HAL_WAIT_ON_BUSY_TIMEOUT_MSEC), k_usleep(100));
    // if (!ret)
    // {
    //     LOG_ERR("Timeout of %dms hit when waiting for sx126x busy!",
    //             CONFIG_LORA_SX128X_HAL_WAIT_ON_BUSY_TIMEOUT_MSEC);
    //     k_oops(); // Terminate the thread
    // }
}

static void sx128x_wakeup_check_ready(const struct device *dev)
{
    const struct sx128x_context_cfg *config = dev->config;
    struct sx128x_context_data *data = dev->data;

    if (data->sleep_status != SX128X_SLEEP) {
        sx128x_hal_wait_on_busy(config);
    } else {
        // Busy is HIGH in sleep mode, wake-up the device with a small glitch on CS
        const struct gpio_dt_spec *cs = &(config->spi.config.cs.gpio);

        gpio_pin_set_dt(cs, 1);
        k_usleep(100);
        gpio_pin_set_dt(cs, 0);
        sx128x_hal_wait_on_busy(config);
        data->sleep_status = SX128X_AWAKE;
    }
}

sx128x_hal_status_t sx128x_hal_write(const void *context, const uint8_t *command,
                                     const uint16_t command_length, const uint8_t *data,
                                     const uint16_t data_length)
{
    PARSE_AND_VALIDATE_CTX(context);

    sx128x_wakeup_check_ready(dev);

    const struct spi_buf tx_bufs[] = {
        {.buf = (uint8_t *)command, .len = command_length},
        {.buf = (uint8_t *)data, .len = data_length},
    };

    const struct spi_buf_set tx_buf_set = {
        .buffers = tx_bufs,
        .count = ARRAY_SIZE(tx_bufs),
    };

    int ret = spi_write_dt(&config->spi, &tx_buf_set);
    if (ret < 0) {
        LOG_ERR("Failed to write to SPI device: %d", ret);
        return SX128X_HAL_STATUS_ERROR;
    }

    // 0x84 is the command to set the radio in sleep mode
    // do not call wakeup after this command
    if (command[0] == 0x84) {
        dev_data->sleep_status = SX128X_SLEEP;
        k_usleep(500); // TODO: check why this is needed
    } else {
        sx128x_wakeup_check_ready(dev);
    }

    return SX128X_HAL_STATUS_OK;
}

sx128x_hal_status_t sx128x_hal_read(const void *context, const uint8_t *command,
                                    const uint16_t command_length, uint8_t *data,
                                    const uint16_t data_length)
{
    PARSE_AND_VALIDATE_CTX(context);

    sx128x_wakeup_check_ready(dev);

    const struct spi_buf tx_bufs[] = {
        {.buf = (uint8_t *)command, .len = command_length},
        {.buf = NULL, .len = data_length},
    };

    const struct spi_buf rx_bufs[] = {
        {.buf = NULL, .len = command_length},
        {.buf = data, .len = data_length},
    };

    const struct spi_buf_set tx_buf_set = {.buffers = tx_bufs, .count = ARRAY_SIZE(tx_bufs)};
    const struct spi_buf_set rx_buf_set = {.buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs)};

    int ret = spi_transceive_dt(&config->spi, &tx_buf_set, &rx_buf_set);
    if (ret < 0) {
        LOG_ERR("Failed to read from SPI device: %d", ret);
        return SX128X_HAL_STATUS_ERROR;
    }

    return SX128X_HAL_STATUS_OK;
}

sx128x_hal_status_t sx126x_hal_reset(const void *context)
{
    PARSE_AND_VALIDATE_CTX(context);
    const struct gpio_dt_spec *nrst = &(config->reset);

    gpio_pin_set_dt(nrst, 1);
    k_msleep(5);
    gpio_pin_set_dt(nrst, 0);
    k_msleep(5);

    dev_data->sleep_status = SX128X_AWAKE;
    return SX128X_HAL_STATUS_OK;
}

sx128x_hal_status_t sx128x_hal_wakeup(const void *context)
{
    PARSE_AND_VALIDATE_CTX(context);
    sx128x_wakeup_check_ready(dev);
    return SX128X_HAL_STATUS_OK;
}
