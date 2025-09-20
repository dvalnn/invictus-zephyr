// HYDRA GPIO abstraction implementation
#include "gpio.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hydra_gpio, LOG_LEVEL_INF);

// Map logical valves to device-tree nodes by label here, so higher layers
// don't need to include Zephyr GPIO headers.
static const struct gpio_dt_spec s_valve_specs[] = {
	GPIO_DT_SPEC_GET(DT_NODELABEL(sol_valve_1), gpios),
	GPIO_DT_SPEC_GET(DT_NODELABEL(sol_valve_2), gpios),
	GPIO_DT_SPEC_GET(DT_NODELABEL(sol_valve_3), gpios),
	GPIO_DT_SPEC_GET(DT_NODELABEL(qdc_n2o), gpios),
	GPIO_DT_SPEC_GET(DT_NODELABEL(qdc_n2), gpios),
	GPIO_DT_SPEC_GET(DT_NODELABEL(st_valve_1), gpios),
	GPIO_DT_SPEC_GET(DT_NODELABEL(st_valve_2), gpios),
};

#define HYDRA_VALVE_GPIO_COUNT (int)(sizeof(s_valve_specs) / sizeof(s_valve_specs[0]))

int gpio_init_valves()
{
	int rc = 0;
	for (int i = 0; i < HYDRA_VALVE_GPIO_COUNT; ++i) {
		const struct gpio_dt_spec *spec = &s_valve_specs[i];
		if (!device_is_ready(spec->port)) {
			LOG_ERR("Valve GPIO dev %d not ready", i);
			rc = -ENODEV;
			continue;
		}
		int r = gpio_pin_configure_dt(spec, GPIO_OUTPUT_INACTIVE);
		if (r) {
			LOG_ERR("Valve GPIO %d configure failed: %d", i, r);
			rc = r;
		}
	}
	if (rc == 0) {
		LOG_INF("Valve GPIOs initialized (%d)", HYDRA_VALVE_GPIO_COUNT);
	}
	return rc;
}

int gpio_set_valve(int id, bool open)
{
	if (id < 0 || id >= HYDRA_VALVE_GPIO_COUNT) {
		return -EINVAL;
	}
	const struct gpio_dt_spec *spec = &s_valve_specs[id];
	if (!device_is_ready(spec->port)) {
		return -ENODEV;
	}
	return gpio_pin_set_dt(spec, open ? 1 : 0);
}

