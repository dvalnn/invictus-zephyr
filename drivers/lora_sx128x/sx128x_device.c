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

#define SX128X_SPI_OPERATION (SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB)

#define RETURN_ON_ERROR(status)          \
  if (status != SX128X_STATUS_OK) {      \
      return init_status;                \
  }

static int sx128x_init(const struct device *dev)
{
    // TODO: Implement the initialization of the device
    // NOTE: Followed datasheet section "14.4 Lora Operation"
    sx128x_status_t init_status; 

    // 1. Set in standby mode if not already
    init_status = sx128x_set_standby(dev, SX128X_STANDBY_CFG_RC);
    RETURN_ON_ERROR(init_status);

    // 2. Set packet type as lora
    init_status = sx128x_set_pkt_type(dev, SX128X_PKT_TYPE_LORA);
    RETURN_ON_ERROR(init_status);

    // 3. Set rf frequency
    // 4. Set base adresses for Rx and Tx (Either full 256MB for both or 128MB each)
    // 5. Set modulation parameter
   
    // 6. Set packet parameters
    static const sx128x_pkt_params_lora_t params =
    {
         .preamble_len = {12}, // recomended value. FIXME: exponent and mantissa
         .header_type = SX128X_LORA_RANGING_PKT_IMPLICIT,
         .pld_len_in_bytes = 253, // 255 - CRC length. TODO confirm
         .crc_is_on = true,
         .invert_iq_is_on = false
    };

    init_status = sx128x_set_lora_pkt_params(dev, &params);
    RETURN_ON_ERROR(init_status);

    return SX128X_STATUS_OK;
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
