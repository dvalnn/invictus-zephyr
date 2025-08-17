// Loosely based on
// https://github.com/zephyrproject-rtos/zephyr/blob/main/drivers/lora/sx12xx_common.c

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/lora.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sx128x_device, CONFIG_LORA_SX128X_LOG_LEVEL);

#include <invictus2/drivers/sx128x.h>
#include <invictus2/drivers/sx128x_context.h>
#include <invictus2/drivers/sx128x_hal.h>

#define SX128X_SPI_OPERATION (SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB)

// TODO make kconfig
#define LORA_SX128X_USE_SPLIT_BUFFER  true
// 256MB
#define LORA_SX128X_BUFFER_SIZE_BYTES 256U

#define RETURN_ON_ERROR(status)                                                               \
    if (status != SX128X_STATUS_OK) {                                                         \
        return status;                                                                        \
    }

static struct sx1280_data {
    const struct device *dev;
    void (*rx_callback)(uint8_t *, uint16_t);
} dev_data;

int _lora_config(const struct device *dev)
{
    LOG_INF("sx128x: init");
    // TODO: Implement the initialization of the device
    // NOTE: Followed datasheet section "14.4 Lora Operation"
    sx128x_status_t init_status;

    // 1. Set in standby mode if not already
    LOG_DBG("setting standby mode");
    init_status = sx128x_set_standby(dev, SX128X_STANDBY_CFG_RC);
    RETURN_ON_ERROR(init_status);

    // 2. Set packet type as lora
    LOG_DBG("setting lora packet type");
    init_status = sx128x_set_pkt_type(dev, SX128X_PKT_TYPE_LORA);
    RETURN_ON_ERROR(init_status);

    // 3. Set rf frequency
    LOG_DBG("setting 2.4GHz");
    init_status = sx128x_set_rf_freq(dev, 2400000000);
    RETURN_ON_ERROR(init_status);

    // 4. Set base adresses for Rx and Tx (Either full 256MB for both or 128MB each)
    const uint8_t tx_base = 0;
    uint8_t rx_base = 0;
    if (LORA_SX128X_USE_SPLIT_BUFFER) {
        rx_base = tx_base + (LORA_SX128X_BUFFER_SIZE_BYTES / 2);
    }

    LOG_DBG("setting buffer addresses to Tx %u and Rx %u", tx_base, rx_base);
    init_status = sx128x_set_buffer_base_address(dev, tx_base, rx_base);
    RETURN_ON_ERROR(init_status);

    // 5. Set modulation parameter
    // TODO: make these parameters a kConfig as we might need to experiment with them
    static const sx128x_mod_params_lora_t mod_params = {
        .sf = SX128X_LORA_RANGING_SF6,
        .bw = SX128X_LORA_RANGING_BW_800,
        .cr = SX128X_LORA_RANGING_CR_4_7,
    };

    LOG_DBG("setting modulation parameters");
    init_status = sx128x_set_lora_mod_params(dev, &mod_params);
    RETURN_ON_ERROR(init_status);

    // 6. Set packet parameters
    static const sx128x_pkt_params_lora_t params = {
        .preamble_len = {12}, // recomended value. FIXME: exponent and mantissa
        .header_type = SX128X_LORA_RANGING_PKT_IMPLICIT,
        // According to datasheet (DS_SX1280-1_V3.3) Table 14-52
        // this value should be 253 at most with CDC enabled
        .pld_len_in_bytes = 253,
        .crc_is_on = true,
        .invert_iq_is_on = false};

    LOG_DBG("configuring packets");
    init_status = sx128x_set_lora_pkt_params(dev, &params);
    RETURN_ON_ERROR(init_status);

    // 7. Go back to sleep
    init_status = sx128x_set_sleep(dev_data.dev, true, true);
    RETURN_ON_ERROR(init_status);

    LOG_INF("finished configuration");
    return SX128X_STATUS_OK;
}

static void _cb_on_recv_event(uint8_t *payload, size_t size)
{
    if (dev_data.rx_callback == NULL) {
        return;
    }

    dev_data.rx_callback(payload, size);

    // start Rx again
    sx128x_set_rx(dev_data.dev, 0, 0);

    // clear IRQ status
    // sx128x_clear_irq_status(dev_data.dev, const sx128x_irq_mask_t irq_mask)
}

void sx1280x_write(uint8_t *payload, size_t size)
{
    // TODO; implement
}

void sx128x_register_recv_callback(void (*rx_callback)(uint8_t *, uint16_t))
{
    dev_data.rx_callback = rx_callback;

    // start reception
    sx128x_set_rx(dev_data.dev, 0, 0);
    //  Configure IRQ to call _cb_on_recv_event
}

static int sx128x_init(const struct device *dev)
{
    dev_data.dev = dev;
    // TODO check SPI pins, etc
    return _lora_config(dev);
}

#define SX128X_DEFINE(node_id)                                                                \
    static struct sx128x_context_data sx128x_data_##node_id;                                  \
                                                                                              \
    static const struct sx128x_context_cfg sx128x_config_##node_id = {                        \
        .spi = SPI_DT_SPEC_GET(node_id, SX128X_SPI_OPERATION, 0),                             \
        .reset = GPIO_DT_SPEC_GET(node_id, reset_gpios),                                      \
        .busy = GPIO_DT_SPEC_GET(node_id, busy_gpios),                                        \
    };                                                                                        \
                                                                                              \
    DEVICE_DT_DEFINE(node_id, sx128x_init, NULL, &sx128x_data_##node_id,                      \
                     &sx128x_config_##node_id, POST_KERNEL, CONFIG_LORA_SX128X_INIT_PRIORITY, \
                     NULL);

DT_FOREACH_STATUS_OKAY(semtech_lora_sx1280, SX128X_DEFINE)
DT_FOREACH_STATUS_OKAY(semtech_lora_sx1281, SX128X_DEFINE)
