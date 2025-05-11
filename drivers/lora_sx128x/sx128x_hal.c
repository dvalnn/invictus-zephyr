#include <invictus2/drivers/sx128x_hal.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sx128x_hal, CONFIG_LORA_SX128X_LOG_LEVEL);

sx128x_hal_status_t sx128x_hal_write(const void *context, const uint8_t *command,
				     const uint16_t command_length, const uint8_t *data,
				     const uint16_t data_length)
{
	return SX128X_HAL_STATUS_ERROR;
}

sx128x_hal_status_t sx128x_hal_read(const void *context, const uint8_t *command,
				    const uint16_t command_length, uint8_t *data,
				    const uint16_t data_length)
{
	return SX128X_HAL_STATUS_ERROR;
}

sx128x_hal_status_t sx128x_hal_reset(const void *context)
{
	return SX128X_HAL_STATUS_ERROR;
}

sx128x_hal_status_t sx128x_hal_wakeup(const void *context)
{
	return SX128X_HAL_STATUS_ERROR;
}
