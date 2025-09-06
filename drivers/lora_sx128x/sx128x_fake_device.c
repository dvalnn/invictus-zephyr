#include <assert.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sx128x_device, CONFIG_LORA_SX128X_LOG_LEVEL);

#include <invictus2/drivers/sx128x.h>
#include <invictus2/drivers/sx128x_context.h>
#include <invictus2/drivers/sx128x_hal.h>
#include <zephyr/drivers/lora.h>

#define DT_DRV_COMPAT zephyr_fake_sx128x

static struct sx1280_data
{
    const struct device *dev;
    void (*rx_callback)(uint8_t *, uint16_t);
} dev_data;

const struct device *fake_sx128x_get_device()
{
    return dev_data.dev;
}

bool fake_sx128x_is_rx_callback_set()
{
    return dev_data.rx_callback != NULL;
}

void fake_sx128x_rf_reception(const uint8_t *bfr, const size_t size)
{
    if (dev_data.rx_callback == NULL)
    {
        LOG_WRN("No callback defined");
        return;
    }

    LOG_DBG("Calling reception callback");
    dev_data.rx_callback(bfr, size);
}

static int sx128x_init(const struct device *dev)
{
    LOG_INF("fake_sx128x: init");
    dev_data.dev = dev;
    LOG_INF("fake_sx128x: init sucess");
    return SX128X_STATUS_OK;
}

void sx1280x_write(uint8_t *data, size_t size)
{
}

void sx128x_register_recv_callback(void (*rx_callback)(uint8_t *, uint16_t))
{
    dev_data.rx_callback = rx_callback;
}

#define SX128X_DEFINE(inst)                                                                   \
    static struct sx128x_context_data sx128x_data_##node_id;                                  \
                                                                                              \
    static const struct sx128x_context_cfg sx128x_config_##node_id = {};                      \
                                                                                              \
    DEVICE_DT_INST_DEFINE(inst, sx128x_init, NULL, &sx128x_data_##node_id,                    \
                          &sx128x_config_##node_id, POST_KERNEL,                              \
                          CONFIG_LORA_SX128X_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(SX128X_DEFINE)
