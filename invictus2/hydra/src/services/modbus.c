#include "services/modbus.h"
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

LOG_MODULE_REGISTER(hydra_modbus, LOG_LEVEL_INF);

static int client_iface;

modbus_memory_t modbus_memory = {0};

actuators_t actuators;
sensors_t sensors;

const static struct modbus_iface_param client_param = {
	.mode = MODBUS_MODE_RTU,
	.rx_timeout = 50000,
	.serial = {
		.baud = 19200,
		.parity = UART_CFG_PARITY_NONE,
	},
};

#define MODBUS_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_modbus_serial)

static int init_modbus_client(void)
{
	const char iface_name[] = {DEVICE_DT_NAME(MODBUS_NODE)};

	client_iface = modbus_iface_get_by_name(iface_name);

	return modbus_init_client(client_iface, client_param);
}


int modbus_init(void)
{
    return init_modbus_client();
}

int read_valve_states() {
	return modbus_read_coils(client_iface, 1, 0, modbus_memory.coils, ACTUATOR_COUNT);
}

int write_valve_state(actuators_t valve, bool state) {
	return modbus_write_coil(client_iface, 1, valve, state);
}

