/*
 * Copyright (c) 2025 Tiago Amorim
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT maxim_max31856

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

// MAX31856 Register Addresses
#define MAX31856_REG_CR0   0x00
#define MAX31856_REG_CR1   0x01
#define MAX31856_REG_LTCB  0x0C // Linearized Thermocouple MSB
#define MAX31856_REG_CJTH  0x0A // Cold-Junction MSB
#define MAX31856_REG_FAULT 0x0F

// Configuration 0 Register Masks
#define MAX31856_CR0_OCONV_MASK 0x80 // One-shot conversion mask
#define MAX31856_CR0_50HZ_MASK  0x01 // 50Hz filter mask

// Configuration 1 Register Masks
#define MAX31856_CR1_TC_TYPE_MASK 0x07 // Thermocouple type selection mask

// MAX31856 Thermocouple Resolution in micro-degrees Celsius (0.0078125 * 1000000)
#define THERMOCOUPLE_RESOLUTION  7812
// MAX31856 Cold-Junction Resolution in micro-degrees Celsius (0.015625 * 1000000)
#define COLD_JUNCTION_RESOLUTION 15625

// Structure for device configuration
struct max31856_config
{
    struct spi_dt_spec spi;
    uint8_t thermocouple_type;
};

// Structure for device data
struct max31856_data
{
    int32_t thermocouple_temp;
    int32_t cold_junction_temp;
};

static int max31856_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
    struct max31856_data *data = dev->data;
    const struct max31856_config *config = dev->config;
    int ret;

    __ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

    // Command to trigger a one-shot conversion
    uint8_t tx_cr0[] = {MAX31856_REG_CR0 | 0x80, MAX31856_CR0_OCONV_MASK};
    struct spi_buf tx_buf = {.buf = tx_cr0, .len = 2};
    struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
    ret = spi_write_dt(&config->spi, &tx);
    if (ret < 0)
    {
        LOG_ERR("Failed to start one-shot conversion (%d)", ret);
        return ret;
    }

    // Wait for conversion to complete (DRDY pin could also be used here)
    k_sleep(K_MSEC(220));

    // Read Linearized Thermocouple Temperature (3 bytes)
    uint8_t tx_ltc[] = {MAX31856_REG_LTCB & 0x7F}; // Read command for register 0x0C
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
    data->thermocouple_temp = sys_get_be24(rx_ltc);

    // Read Cold-Junction Temperature (2 bytes)
    uint8_t tx_cj[] = {MAX31856_REG_CJTH & 0x7F}; // Read command for register 0x0A
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
    data->cold_junction_temp = sys_get_be16(rx_cj);

    // Read Fault Status Register
    uint8_t tx_fault[] = {MAX31856_REG_FAULT & 0x7F}; // Read command for register 0x0F
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
        LOG_ERR("MAX31856 fault detected: %02x", rx_fault[0]);
        // Return a specific error code based on the fault flags
        return -EIO;
    }

    return 0;
}

static int max31856_channel_get(const struct device *dev, enum sensor_channel chan,
                                struct sensor_value *val)
{
    struct max31856_data *data = dev->data;
    int32_t temp;
    int64_t micro_degrees;

    switch (chan)
    {
    case SENSOR_CHAN_AMBIENT_TEMP:
        // Thermocouple temperature (19-bit, signed)
        temp = data->thermocouple_temp;
        // The value is 19 bits, with sign bit at MSB (bit 18).
        // If the MSB is 1, the value is negative.
        if (temp & BIT(18))
        {
            temp |= 0xFFF80000; // Sign extend to 32 bits
        }
        micro_degrees = (int64_t)temp * THERMOCOUPLE_RESOLUTION;
        val->val1 = micro_degrees / 1000000;
        val->val2 = micro_degrees % 1000000;
        break;

    case SENSOR_CHAN_DIE_TEMP:
        // Cold-junction temperature (16-bit, signed)
        temp = data->cold_junction_temp;
        micro_degrees = (int64_t)temp * COLD_JUNCTION_RESOLUTION;
        val->val1 = micro_degrees / 1000000;
        val->val2 = micro_degrees % 1000000;
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

static int max31856_init(const struct device *dev)
{
    const struct max31856_config *config = dev->config;
    int ret;

    if (!spi_is_ready_dt(&config->spi))
    {
        LOG_ERR("SPI bus is not ready");
        return -ENODEV;
    }

    // Configure the MAX31856 for continuous conversion and thermocouple type
    // Write to Configuration 0 and Configuration 1 registers
    uint8_t config_tx[] = {// Register 00h (Configuration 0):
                           // 0x00 | (0 << 7) -> Continuous conversion mode (CMODE=0)
                           // | (0 << 6) -> No fault clear (FCLR=0)
                           // | (0 << 5) -> Open circuit detection disabled (OCDET=0)
                           // | (0 << 4) -> Internal Cold-Junction (CJ=0)
                           // | (0 << 3) -> Average mode disabled (AVG=0)
                           // | (0 << 2) -> Fault interrupt enabled (FAULT_INT=0)
                           // | (0 << 1) -> No bias voltage (BIAS=0)
                           // | (0 << 0) -> 60Hz filtering (50HZ=0)
                           MAX31856_REG_CR0, 0x00,
                           // Register 01h (Configuration 1):
                           // 0x01 | (0 << 7) -> No fault checking (FAULT_CHECK=0)
                           // | (0 << 6-4) -> No averaging (AVG=0)
                           // | (0 << 3) -> No VMODE
                           // | (thermocouple_type & 0x07) -> Set TC type
                           MAX31856_REG_CR1, config->thermocouple_type & 0x07};
    struct spi_buf tx_buf = {.buf = config_tx, .len = sizeof(config_tx)};
    const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};

    ret = spi_write_dt(&config->spi, &tx_set);
    if (ret < 0)
    {
        LOG_ERR("Failed to configure MAX31856 (%d)", ret);
        return ret;
    }

    return 0;
}

#define MAX31856_INIT(n)                                                                      \
    static struct max31856_data max31856_data_##n;                                            \
    static const struct max31856_config max31856_config_##n = {                               \
        .spi = SPI_DT_SPEC_INST_GET(n, SPI_WORD_SET(8U) | SPI_TRANSFER_MSB, 0),               \
        .thermocouple_type = DT_INST_PROP_OR(n, thermocouple_type, 4),                        \
    };                                                                                        \
    SENSOR_DEVICE_DT_INST_DEFINE(n, &max31856_init, NULL, &max31856_data_##n,                 \
                                 &max31856_config_##n, POST_KERNEL,                           \
                                 CONFIG_SENSOR_INIT_PRIORITY, &max31856_api);

DT_INST_FOREACH_STATUS_OKAY(MAX31856_INIT)
