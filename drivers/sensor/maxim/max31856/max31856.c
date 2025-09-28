/*
 * Copyright (c) 2025 Tiago Amorim
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT maxim_max31856

#include <invictus2/drivers/sensor/maxim/max31856/max31856.h>
#include <invictus2/dt-bindings/sensor/maxim/max31856.h>

#include <stdint.h>

#include <zephyr/init.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(MAX31856, CONFIG_SENSOR_LOG_LEVEL);

// Resolution constants (in microdegrees Celsius)
#define THERMOCOUPLE_RESOLUTION_UDC  7812
#define COLD_JUNCTION_RESOLUTION_UDC 15625

#define SPI_WRITE_MASK BIT(7)
#define SPI_READ_MASK  GENMASK(6, 0)

// Structure for device data
static int max31856_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
    struct max31856_data *data = dev->data;
    const struct max31856_config *config = dev->config;
    int ret;

    __ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

    /* // Command to trigger a one-shot conversion */
    /* uint8_t tx_cr0[] = {MAX31856_CR0_REG | SPI_WRITE_MASK, 0x1 << MAX31856_CR0_1SHOT}; */
    /* struct spi_buf tx_buf = {.buf = tx_cr0, .len = 2}; */
    /* struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1}; */
    /* ret = spi_write_dt(&config->spi, &tx); */
    /* if (ret < 0) */
    /* { */
    /*     LOG_ERR("Failed to start one-shot conversion (%d)", ret); */
    /*     return ret; */
    /* } */

    /* // Wait for conversion to complete (DRDY pin could also be used here) */
    /* k_sleep(K_MSEC(220)); */

    // Read Linearized Thermocouple Temperature (3 bytes)
    uint8_t tx_ltc[] = {MAX31856_LTCBH_REG & SPI_READ_MASK}; // Read command for register 0x0C
    uint8_t rx_ltc[3];
    struct spi_buf tx_ltc_buf = {.buf = tx_ltc, .len = 1};
    struct spi_buf rx_ltc_buf = {.buf = rx_ltc, .len = 3};
    const struct spi_buf_set tx_ltc_set = {.buffers = &tx_ltc_buf, .count = 1};
    const struct spi_buf_set rx_ltc_set = {.buffers = &rx_ltc_buf, .count = 1};
    ret = spi_transceive_dt(&config->spi, &tx_ltc_set, &rx_ltc_set);
    if (ret < 0)
    {
        LOG_ERR("Failed to read thermocouple temperature (%d)", ret);
        return ret;
    }

    data->thermocouple_temp = (rx_ltc[0] << 16) | (rx_ltc[1] << 8) | rx_ltc[2];

    // Read Cold-Junction Temperature (2 bytes)
    uint8_t tx_cj[] = {MAX31856_CJTH_REG & SPI_READ_MASK}; // Read command for register 0x0A
    uint8_t rx_cj[2];
    struct spi_buf tx_cj_buf = {.buf = tx_cj, .len = 1};
    struct spi_buf rx_cj_buf = {.buf = rx_cj, .len = 2};
    const struct spi_buf_set tx_cj_set = {.buffers = &tx_cj_buf, .count = 1};
    const struct spi_buf_set rx_cj_set = {.buffers = &rx_cj_buf, .count = 1};

    ret = spi_transceive_dt(&config->spi, &tx_cj_set, &rx_cj_set);
    if (ret < 0)
    {
        LOG_ERR("Failed to read cold-junction temperature (%d)", ret);
        return ret;
    }
    data->cold_junction_temp = rx_cj[0] << 8 | rx_cj[1];

    // Read Fault Status Register
    uint8_t tx_fault[] = {MAX31856_SR_REG & SPI_READ_MASK}; // Read command for register 0x0F
    uint8_t rx_fault[1];
    struct spi_buf tx_fault_buf = {.buf = tx_fault, .len = 1};
    struct spi_buf rx_fault_buf = {.buf = rx_fault, .len = 1};
    const struct spi_buf_set tx_fault_set = {.buffers = &tx_fault_buf, .count = 1};
    const struct spi_buf_set rx_fault_set = {.buffers = &rx_fault_buf, .count = 1};

    ret = spi_transceive_dt(&config->spi, &tx_fault_set, &rx_fault_set);
    if (ret < 0)
    {
        LOG_ERR("Failed to read fault register (%d)", ret);
        return ret;
    }
    if (rx_fault[0] != 0)
    {
        LOG_WRN("fault detected on %s: %02x", dev->name, rx_fault[0]);
    }

    return 0;
}

static int max31856_channel_get(const struct device *dev, enum sensor_channel chan,
                                struct sensor_value *val)
{
    struct max31856_data *data = dev->data;
    switch (chan)
    {
    case SENSOR_CHAN_AMBIENT_TEMP:
    {
        // Thermocouple temperature (19-bit, signed)
        int32_t temperature = (data->thermocouple_temp >> 5) & GENMASK(18, 0);
        if (temperature & BIT(18))
        {
            temperature |= 0xFFF80000; // Sign extend to 32 bits
        }

        int64_t micro_degrees = (int64_t)temperature * THERMOCOUPLE_RESOLUTION_UDC;
        val->val1 = micro_degrees / 1000000;
        val->val2 = llabs(micro_degrees % 1000000);
    }
    break;

    case SENSOR_CHAN_DIE_TEMP:
    {
        int16_t cj_temp = (int16_t)data->cold_junction_temp;
        int64_t micro_degrees = (int64_t)cj_temp * COLD_JUNCTION_RESOLUTION_UDC;
        val->val1 = micro_degrees / 1000000;
        val->val2 = llabs(micro_degrees % 1000000);
    } // Cold-junction temperature (16-bit, signed)
    break;

    default:
        return -ENOTSUP;
    }

    return 0;
}

static const struct sensor_driver_api max31856_api = {
    .sample_fetch = max31856_sample_fetch,
    .channel_get = max31856_channel_get,
};

static void check_config(const struct device *dev)
{
    const struct max31856_config *const cfg = dev->config;
    uint8_t tx_buf[] = {MAX31856_CR0_REG & SPI_READ_MASK, 0x00, 0x00, 0x00};
    uint8_t rx_buf[] = {0x00, 0x00, 0x00, 0x00};

    struct spi_buf tx_bufs[] = {
        {.buf = tx_buf, .len = sizeof(tx_buf)},
    };

    struct spi_buf rx_bufs[] = {
        {.buf = rx_buf, .len = sizeof(rx_buf)},
    };

    const struct spi_buf_set tx_buf_set = {.buffers = tx_bufs, .count = 1};
    const struct spi_buf_set rx_buf_set = {.buffers = rx_bufs, .count = 1};

    int ret = spi_transceive_dt(&cfg->spi, &tx_buf_set, &rx_buf_set);
    if (ret < 0)
    {
        LOG_ERR("Failed to read back config (%d)", ret);
        return;
    }

    LOG_HEXDUMP_DBG(rx_buf, sizeof(rx_buf), "Config state");
}

static int write_init_config(const struct device *dev, uint8_t *config, size_t len)
{
    const struct max31856_config *const cfg = dev->config;
    int ret;

    struct spi_buf tx_bufs[] = {
        {.buf = config, .len = len},
    };

    const struct spi_buf_set tx_set = {.buffers = tx_bufs, .count = 1};
    ret = spi_write_dt(&cfg->spi, &tx_set);
    if (ret < 0)
    {
        LOG_ERR("Failed to write init config (%d)", ret);
        return ret;
    }

    LOG_HEXDUMP_DBG(tx_bufs[0].buf, tx_bufs[0].len, "Written");
    return 0;
}

static int max31856_init(const struct device *const dev)
{
    const struct max31856_config *const cfg = dev->config;
    int ret;

    if (!spi_is_ready_dt(&cfg->spi))
    {
        LOG_ERR("SPI bus is not ready");
        return -ENODEV;
    }

    if (cfg->has_fault_gpio)
    {
        if (!gpio_is_ready_dt(&cfg->fault_gpio))
        {
            LOG_ERR("Fault GPIO not ready");
            return -ENODEV;
        }

        ret = gpio_pin_configure_dt(&cfg->fault_gpio, GPIO_INPUT);
        if (ret < 0)
        {
            LOG_ERR("Failed to configure Fault GPIO (%d)", ret);
            return ret;
        }
    }

    if (cfg->has_drdy_gpio)
    {
        if (!gpio_is_ready_dt(&cfg->drdy_gpio))
        {
            LOG_ERR("DRDY GPIO not ready");
            return -ENODEV;
        }

        ret = gpio_pin_configure_dt(&cfg->drdy_gpio, GPIO_INPUT);
        if (ret < 0)
        {
            LOG_ERR("Failed to configure DRDY GPIO (%d)", ret);
            return ret;
        }
    }

    // Configure the MAX31856 for continuous conversion and thermocouple type
    // Write to Configuration 0 and Configuration 1 registers

    // clang-format off
    uint8_t config_tx[] = {
        // Register 00h (Configuration 0):
        MAX31856_CR0_REG | SPI_WRITE_MASK,
        0x00
        | ((0x1                 << MAX31856_CR0_AUTOCONVERT) & BIT(7))
        | ((0x0                 << MAX31856_CR0_1SHOT      ) & BIT(6))
        | ((cfg->oc_fault_mode  << MAX31856_CR0_OCFAULT    ) & GENMASK(5,4))
        | ((0x0                 << MAX31856_CR0_CJ         ) & BIT(3))
        | ((cfg->has_fault_gpio << MAX31856_CR0_FAULT      ) & BIT(2))
        | ((cfg->has_fault_gpio << MAX31856_CR0_FAULTCLR   ) & BIT(1))
        | ((cfg->filter_50hz    << MAX31856_CR0_50HZ       ) & BIT(0)),

        /* Register 01h (Configuration 1): */
        /* MAX31856_CR1_REG | SPI_WRITE_MASK, */
        0x00
        | ((cfg->avg_mode          << MAX31856_CR1_AVG_SEL) & GENMASK(6,4))
        | ((cfg->thermocouple_type << MAX31856_CR1_TCTYPE ) & GENMASK(3,0)),

        /* Register 02h (Fault Mask): */
        /* MAX31856_MASK_REG | SPI_WRITE_MASK, */
        0x00, // Mask all faults (disable fault detection)
    };

    LOG_INF("Initializing MAX31856 %s", dev->name);
    ret = write_init_config(dev, config_tx, sizeof(config_tx));
    if (ret < 0)
    {
        return ret;
    }
    check_config(dev);

    uint8_t config_cjto[] = {
        // Register 09h (Cold-Junction Temperature Offset):
        MAX31856_CJTO_REG | SPI_WRITE_MASK,
        cfg->cj_offset_raw,
    };

    ret = spi_write_dt(&cfg->spi, &(struct spi_buf_set){
        .buffers = (struct spi_buf[]){
            {.buf = config_cjto, .len = sizeof(config_cjto)},
        },
        .count = 1,
    });

    LOG_INF("MAX31856 %s INIT END", dev->name);
    return 0;
}

#define SPI_FLAGS (SPI_WORD_SET(8U) | SPI_TRANSFER_MSB | SPI_MODE_CPHA)

#define MAX31856_INIT(n)                                                                      \
    static struct max31856_data max31856_data_##n;                                            \
    static const struct max31856_config max31856_config_##n = {                               \
        .spi = SPI_DT_SPEC_INST_GET(n, SPI_FLAGS, 0),                                         \
        .drdy_gpio = GPIO_DT_SPEC_INST_GET_OR(n, drdy_gpios, {0}),                            \
        .has_drdy_gpio = DT_INST_NODE_HAS_PROP(n, drdy_gpios),                                \
        .fault_gpio = GPIO_DT_SPEC_INST_GET_OR(n, fault_gpios, {0}),                          \
        .has_fault_gpio = DT_INST_NODE_HAS_PROP(n, fault_gpios),                              \
        .thermocouple_type = DT_INST_PROP_OR(n, thermocouple_type, 4),                        \
        .filter_50hz = DT_INST_PROP_OR(n, filter_50hz, false),                                \
        .avg_mode = DT_INST_PROP_OR(n, averaging_mode, MAX31856_AVG_MODE_1),                  \
        .oc_fault_mode =                                                                      \
            DT_INST_PROP_OR(n, open_circuit_mode, MAX31856_OCFAULT_DET_DISABLED),             \
        .cj_offset_raw = DT_INST_PROP_OR(n, cj_offset, 0),                                    \
    };                                                                                        \
    SENSOR_DEVICE_DT_INST_DEFINE(n, &max31856_init, NULL, &max31856_data_##n,                 \
                                 &max31856_config_##n, POST_KERNEL,                           \
                                 CONFIG_SENSOR_INIT_PRIORITY, &max31856_api);

DT_INST_FOREACH_STATUS_OKAY(MAX31856_INIT)
