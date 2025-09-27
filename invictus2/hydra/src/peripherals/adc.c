#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

#include "peripherals/adc.h"

// FIXME: Remove later
#include "pressures.h"

LOG_MODULE_REGISTER(adc_sample, LOG_LEVEL_INF);

#define ADC_NODE             DT_NODELABEL(adc) // adjust this to match your board’s ADC node
#define ADC_CHANNEL          1                 // ADC channel number you want to read
#define ADC_RESOLUTION       12                // bits of resolution
#define ADC_GAIN             ADC_GAIN_1
#define ADC_REFERENCE        ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME_DEFAULT

static const struct device *adc_dev = DEVICE_DT_GET(ADC_NODE);

static int16_t sample_buffer; // buffer for ADC result

static struct adc_channel_cfg channel_cfg;
struct adc_sequence sequence;

void init_adcs()
{
    channel_cfg.gain = ADC_GAIN;
    channel_cfg.reference = ADC_REFERENCE;
    channel_cfg.acquisition_time = ADC_ACQUISITION_TIME;
    channel_cfg.channel_id = ADC_CHANNEL;
    channel_cfg.differential = 0;

    sequence.channels = BIT(ADC_CHANNEL);
    sequence.buffer = &sample_buffer;
    sequence.buffer_size = sizeof(sample_buffer);
    sequence.resolution = ADC_RESOLUTION;

    if (!device_is_ready(adc_dev))
    {
        LOG_ERR("ADC device not ready");
        return;
    }

    int err = adc_channel_setup(adc_dev, &channel_cfg);
    if (err)
    {
        LOG_ERR("Failed to setup ADC channel (%d)", err);
        return;
    }
}

int adc_raw_to_voltage(int16_t raw, int32_t ref_mv, enum adc_gain gain, uint8_t resolution,
                       int32_t *result_mv)
{
    int32_t val = raw;

    int ret = adc_raw_to_millivolts(ref_mv, gain, resolution, &val);
    if (ret < 0)
    {
        return ret;
    }

    *result_mv = val;
    return 0;
}

int read_adcs()
{
    int16_t raw;
    int32_t mv;

    int err = adc_read(adc_dev, &sequence);
    if (err)
    {
        LOG_ERR("ADC read failed (%d)", err);
    }
    else
    {
        raw = sample_buffer;
        int ret = adc_raw_to_voltage(raw,
                                     ADC_REFERENCE,  // reference = 3.3V
                                     ADC_GAIN,       // no scaling
                                     ADC_RESOLUTION, // 12-bit resolution
                                     &mv);
        if (ret < 0)
        {
            LOG_ERR("Failed to convert ADC raw to voltage (%d)", ret);
        }
        LOG_INF("ADC raw value: %d", sample_buffer);
        LOG_INF("ADC voltage: %d mV", mv);
        int mbar = mv_to_mbar(mv, N2_PRESSURE_MIN_BAR, N2_PRESSURE_MAX_BAR);
        LOG_INF("Pressure = %d.%03d bar\n", mbar / 1000, mbar % 1000);
    }
    return err;
}
