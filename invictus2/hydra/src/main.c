#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "valves.h"
#include "peripherals/pwm.h"
#include "peripherals/adc.h"
#include "services/modbus.h"
#include "pressures.h"

#define MAIN_DELAY 1000

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

void setup()
{
    /*
    pwm_init();
    LOG_INF("PWM initialized");
    init_adcs();
    LOG_INF("ADC initialized");
    int rc = valves_init();
    if (rc)
    {
        LOG_WRN("Valves init failed: %d", rc);
    }
    else
    {
        LOG_INF("Valves initialized");
    }
    */

#if DT_NODE_HAS_COMPAT(DT_PARENT(MODBUS_NODE), zephyr_cdc_acm_uart)
	const struct device *const dev = DEVICE_DT_GET(DT_PARENT(MODBUS_NODE));
	uint32_t dtr = 0;

	if (!device_is_ready(dev)) {
		return 0;
	}

	while (!dtr) {
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		k_sleep(K_MSEC(100));
	}

	LOG_INF("Client connected to server on %s", dev->name);
#endif

	if (init_modbus_server()) {
		LOG_ERR("Modbus RTU server initialization failed");
	}
}

void loop()
{
    k_sleep(K_MSEC(MAIN_DELAY));    
}

int main(void)
{
    setup();
    while (1)
    {
        loop();
    }
    k_oops(); // should never reach here
}