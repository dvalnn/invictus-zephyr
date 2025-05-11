#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sx128x_device, CONFIG_LORA_SX128X_LOG_LEVEL);

#include <invictus2/drivers/sx128x_context.h>
#include <invictus2/drivers/sx128x_hal.h>

#define SX128X_SPI_OPERATION (SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB)

static int sx128x_init(const struct device *dev)
{
    // TODO: Implement the initialization of the device
    return 0;
}

#define SX128X_DEFINE(node_id)                                                                     \
    static struct sx128x_context_data sx128x_data_##node_id;                                       \
                                                                                                   \
    static const struct sx128x_context_cfg sx128x_config_##node_id = {                             \
            .spi = SPI_DT_SPEC_GET(node_id, SX128X_SPI_OPERATION, 0),                              \
            .reset = GPIO_DT_SPEC_GET(node_id, reset_gpios),                                       \
            .busy = GPIO_DT_SPEC_GET(node_id, busy_gpios),                                         \
    };                                                                                             \
                                                                                                   \
    DEVICE_DT_DEFINE(node_id, sx128x_init, NULL, &sx128x_data_##node_id, &sx128x_config_##node_id, \
                     POST_KERNEL, CONFIG_LORA_SX128X_INIT_PRIORITY, NULL);

DT_FOREACH_STATUS_OKAY(semtech_lora_sx1280, SX128X_DEFINE)
DT_FOREACH_STATUS_OKAY(semtech_lora_sx1281, SX128X_DEFINE)
