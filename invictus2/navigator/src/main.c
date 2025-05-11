/*
 * DEMO APPLICATION FOR MPU9250 SENSOR.
 * Just trying out the zephyr sensor API, as the MPU9250 will not be used in the final board.
 * This is just a demo application to test the MPU9250 sensor + a blinking LED.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 2000

/* The devicetree node identifier for the "led0" alias. */
#define LED1_NODE DT_ALIAS(led1)

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

static int process_mpu9250(const struct device *dev)
{
	struct sensor_value temperature;
	struct sensor_value accel[3];
	struct sensor_value gyro[3];
	int rc = sensor_sample_fetch(dev);

	if (rc == 0) {
		rc = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);
	}
	if (rc == 0) {
		rc = sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, gyro);
	}
	if (rc == 0) {
		rc = sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &temperature);
	}

	if (rc == 0) {
		LOG_INF("device %s sample fetch/get succeeded", dev->name);
		LOG_INF("temperature: %d.%06d C", temperature.val1, temperature.val2);
		LOG_INF("accel: %d.%06d %d.%06d %d.%06d m/s^2", accel[0].val1, accel[0].val2,
			accel[1].val1, accel[1].val2, accel[2].val1, accel[2].val2);
		LOG_INF("gyro: %d.%06d %d.%06d %d.%06d rad/s", gyro[0].val1, gyro[0].val2,
			gyro[1].val1, gyro[1].val2, gyro[2].val1, gyro[2].val2);
	} else {
		LOG_ERR("device %s sample fetch/get failed: %d\n", dev->name, rc);
	}

	return rc;
}

int main(void)
{
	int ret;
	bool led_state = true;

	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}

	const struct device *const mpu = DEVICE_DT_GET_ONE(invensense_mpu9250);
	if (!device_is_ready(mpu)) {
		LOG_ERR("MPU device %s is not ready", mpu->name);
		return 0;
	}

	while (1) {
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			return 0;
		}

		ret = process_mpu9250(mpu);
		if (ret < 0) {
			LOG_ERR("MPU9250 process failed: %d", ret);
			break;
		}

		led_state = !led_state;
		printk("LED state: %s\n", led_state ? "ON" : "OFF");

		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
