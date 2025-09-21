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

#define RETURN_ON_ERROR(status)                                                               \
    if (status != SX128X_STATUS_OK)                                                           \
    {                                                                                         \
        LOG_ERR("Failed to send command");                                                    \
        return status;                                                                        \
    }

extern void health_check(void);
static struct gpio_callback dio3_cb_data;

static struct sx1280_data
{
    const struct device *dev;
    void (*rx_callback)(uint8_t *, uint16_t);
} dev_data;

void sx128x_log_configuration(const struct device *dev)
{
    uint8_t status;
    sx128x_status_t ret = sx128x_get_lora_pkt_len(dev, &status);
    LOG_INF("pkt len: %d -> %d", (int)ret, (int)status);

    sx128x_pkt_type_t pkt_type;
    ret = sx128x_get_pkt_type(dev, &pkt_type);
    LOG_INF("type: %d -> %d", (int)ret, (int)pkt_type);

    sx128x_lora_pkt_len_modes_t pkt_len;
    ret = sx128x_get_lora_pkt_len_mode(dev, &pkt_len);
    LOG_INF("mode: %d -> %d", (int)ret, (int)pkt_len);
}

// Datasheet section 14.4.3 2. Rx Settings and Operations
// periodBaseCount is set 0xFFFF, Rx Continuous mode, the device remains in Rx mode until the
// host sends a command to change the operation mode. The device can receive several packets.
// Each time a packet is received, a packet received indication is given to the host and the
// device will continue to search for a new packet
int _sx128x_set_continous_rx_mode(const struct device *dev)
{
    sx128x_clear_irq_status(dev, SX128X_IRQ_ALL);
    return sx128x_set_rx(dev_data.dev, SX128X_TICK_SIZE_0015_US, 0xFF);
}

static void _cb_on_recv_event(const struct device *dev, struct gpio_callback *cb,
                              uint32_t pins)
{
    health_check();
    if (dev_data.rx_callback == NULL)
    {
        goto irq_finalize;
    }
    sx128x_status_t ret;
// if (dev_data.rx_callback == NULL)
// {
//     return;
// }
//
// dev_data.rx_callback(payload, size);
//
// // start Rx again
// sx128x_set_rx(dev_data.dev, 0, 0);
//
irq_finalize:
    // clear IRQ status
    ret = sx128x_clear_irq_status(dev, SX128X_IRQ_ALL);
    if (ret != SX128X_STATUS_OK)
    {
        LOG_ERR("failed to clear RX IRQ status");
    }
}

bool _sx128x_configure_peripherals(const struct device *dev)
{
    const struct sx128x_context_cfg *config = dev->config;

    // SPI device
    if (!spi_is_ready_dt(&config->spi))
    {
        LOG_DBG("SPI device not ready");
        return false;
    }

    // Chip select
    if (!spi_cs_is_gpio_dt(&config->spi))
    {
        LOG_ERR("NSS NOT SET");
        return false;
    }

    // Busy GPIO
    if (!gpio_is_ready_dt(&config->busy))
    {
        LOG_ERR("BUSY gpio is not ready");
        return false;
    }
    LOG_DBG("Valid BUSY gpio");

    int ret = gpio_pin_configure_dt(&config->busy, GPIO_INPUT);
    if (ret != 0)
    {
        LOG_ERR("Failed to configure BUSY as input due to %d", ret);
        return false;
    }

    if (!gpio_is_ready_dt(&config->dio3))
    {
        LOG_ERR("DIO gpio is not ready");
        return false;
    }
    LOG_DBG("Valid DIO gpio");

    ret = gpio_pin_configure_dt(&config->dio3, GPIO_INPUT | config->dio3.dt_flags);
    if (ret != 0)
    {
        LOG_ERR("Failed to configure DIO as input due to %d", ret);
        return false;
    }
    LOG_DBG("Configured DIO gpio");

    int callback_result = gpio_pin_interrupt_configure_dt(&config->dio3, GPIO_INT_EDGE_RISING);
    if (callback_result < 0)
    {
        LOG_ERR("Failed to set interrupt due to %d", callback_result);
        return false;
    }
    //  Configure IRQ to call _cb_on_recv_event
    gpio_init_callback(&dio3_cb_data, _cb_on_recv_event, BIT(config->dio3.pin));
    LOG_DBG("Initialized DIO");

    callback_result = gpio_add_callback(config->dio3.port, &dio3_cb_data);
    if (callback_result < 0)
    {
        LOG_ERR("Failed to set callback due to %d", callback_result);
        return false;
    }
    LOG_INF("Configured peripherals");

    return true;
}

int _lora_config(const struct device *dev)
{
    if (!_sx128x_configure_peripherals(dev))
    {
        LOG_ERR("failed peripheral validation");
        return SX128X_STATUS_ERROR;
    }
    /* const struct sx128x_context_cfg *config = dev->config; */

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
    const uint8_t rx_base = tx_base; // + LORA_SX128X_BUFFER_SIZE_BYTES;

    LOG_DBG("setting buffer addresses to Tx %u and Rx %u", tx_base, rx_base);
    init_status = sx128x_set_buffer_base_address(dev, tx_base, rx_base);
    RETURN_ON_ERROR(init_status);

    // 5. Set modulation parameter
    // TODO: make these parameters a kConfig as we might need to experiment with them
    static const sx128x_mod_params_lora_t mod_params = {
        .sf = SX128X_LORA_RANGING_SF12,
        .bw = SX128X_LORA_RANGING_BW_1600,
        .cr = SX128X_LORA_RANGING_CR_LI_4_8,
    };

    LOG_DBG("setting modulation parameters");
    init_status = sx128x_set_lora_mod_params(dev, &mod_params);
    RETURN_ON_ERROR(init_status);

    // 6. Set packet parameters
    static const sx128x_pkt_params_lora_t params = {
        .preamble_len =
            {
                .mant = 6,
                .exp = 0,
            },
        .header_type = SX128X_LORA_RANGING_PKT_IMPLICIT,
        // According to datasheet (DS_SX1280-1_V3.3) Table 14-52
        // this value should be account for CRC (2 bytes)
        .pld_len_in_bytes = 253,
        .crc_is_on = true,
        .invert_iq_is_on = false};

    LOG_DBG("configuring packets");
    init_status = sx128x_set_lora_pkt_params(dev, &params);
    RETURN_ON_ERROR(init_status);

    LOG_DBG("finished configuration");

    sx128x_log_configuration(dev);

    // TODO is it needed to sleep here?
    // // 7. Go back to sleep
    // init_status = sx128x_set_sleep(dev_data.dev, true, true);
    // RETURN_ON_ERROR(init_status);

    return SX128X_STATUS_OK;
}

int sx128x_read(uint8_t *payload, size_t size)
{
    return sx128x_read_buffer(&dev_data.dev, LORA_SX128X_BUFFER_SIZE_BYTES, payload, size);
}

bool sx128x_transmit(const uint8_t *payload, size_t size)
{
    sx128x_status_t ret = sx128x_set_tx_params(dev_data.dev, 13, SX128X_RAMP_20_US);
    if (ret != SX128X_STATUS_OK)
    {
        LOG_ERR("failed to set TX parameters %d: ", ret);
    }

    sx128x_status_t transmission_result = sx128x_write_buffer(dev_data.dev, 0, payload, size);
    if (transmission_result != SX128X_STATUS_OK)
    {
        LOG_ERR("failed to write command");
    }

    ret = sx128x_set_dio_irq_params(dev_data.dev, SX128X_IRQ_ALL, SX128X_IRQ_NONE,
                                    SX128X_IRQ_NONE, SX128X_IRQ_ALL);

    // FIXME should we use a timeout value?
    ret = sx128x_set_tx(dev_data.dev, SX128X_TICK_SIZE_1000_US, 100);
    if (ret != SX128X_STATUS_OK)
    {
        LOG_INF("Failed to transmit");
        return false;
    }

    // go back to RX mode
    // if (_sx128x_set_continous_rx_mode(dev_data.dev) != 0)
    // {
    //     LOG_ERR("Failed to go back to RX mode");
    // }
    return transmission_result == SX128X_STATUS_OK;
}

bool sx128x_register_recv_callback(void (*rx_callback)(uint8_t *, uint16_t))
{
    LOG_INF("Setting up receive callback");
    // configure
    const struct device *dev = (const struct device *)dev_data.dev;

    dev_data.rx_callback = rx_callback;
    LOG_DBG("Configured callback");

    int ret = sx128x_set_dio_irq_params(dev, SX128X_IRQ_RX_DONE, SX128X_IRQ_NONE,
                                        SX128X_IRQ_NONE, SX128X_IRQ_RX_DONE);
    if (ret != SX128X_STATUS_OK)
    {
        LOG_ERR("Failed to configure DIO3 IRQ");
        return false;
    }
    LOG_DBG("Configured DIO IRQ");

    // start reception
    int result = _sx128x_set_continous_rx_mode(dev);
    if (result != 0)
    {
        LOG_ERR("Failed to set RX mode");
        return false;
    }

    sx128x_chip_status_t radio_status;
    sx128x_status_t status_ret = sx128x_get_status(dev, &radio_status);
    if (status_ret != SX128X_STATUS_OK || radio_status.chip_mode != SX128X_CHIP_MODE_RX)
    {
        // TODO: verify because documentation is confusing
        LOG_ERR("%d | %d | %d", status_ret, radio_status.cmd_status, radio_status.chip_mode);
    }
    LOG_DBG("Radio in RX mode");
    return SX128X_STATUS_OK;
}

static int sx128x_init(const struct device *dev)
{
    dev_data.dev = dev;
    dev_data.rx_callback = NULL;

    // TODO check SPI pins, etc
    return _lora_config(dev);
}

#define SX128X_DEFINE(node_id)                                                                \
    static struct sx128x_context_data sx128x_data_##node_id;                                  \
                                                                                              \
    static const struct sx128x_context_cfg sx128x_config_##node_id = {                        \
        .spi = SPI_DT_SPEC_GET(node_id, SX128X_SPI_OPERATION, 0),                             \
        .busy = GPIO_DT_SPEC_GET(node_id, busy_gpios),                                        \
        .dio3 = GPIO_DT_SPEC_GET(node_id, dio3_gpios),                                        \
    };                                                                                        \
                                                                                              \
    DEVICE_DT_DEFINE(node_id, sx128x_init, NULL, &sx128x_data_##node_id,                      \
                     &sx128x_config_##node_id, POST_KERNEL, CONFIG_LORA_SX128X_INIT_PRIORITY, \
                     NULL);

DT_FOREACH_STATUS_OKAY(semtech_lora_sx1280, SX128X_DEFINE)
DT_FOREACH_STATUS_OKAY(semtech_lora_sx1281, SX128X_DEFINE)
