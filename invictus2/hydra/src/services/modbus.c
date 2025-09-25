#include "services/modbus.h"
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/modbus/modbus.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(hydra_modbus, LOG_LEVEL_INF);

static modbus_memory_t mb_mem = {0};

static int coil_rd(uint16_t addr, bool *state)
{
	if (addr >= ACTUATOR_COUNT) {
		return -ENOTSUP;
	}
	if (mb_mem.coils[addr / 8] & BIT(addr % 8)) {
		*state = true;
	} else {
		*state = false;
	}

	LOG_INF("Coil read, addr %u, %d", addr, (int)*state);

	return 0;
}

static int coil_wr(uint16_t addr, bool state)
{
	bool on;
	if (addr >= ACTUATOR_COUNT) {
		return -ENOTSUP;
	}

	if (state == true) {
		mb_mem.coils[addr / 8] |= BIT(addr % 8);
		on = true;
	} else {
		mb_mem.coils[addr / 8] &= ~BIT(addr % 8);
		on = false;
	}

	//gpio_pin_set(led_dev[addr].port, led_dev[addr].pin, (int)on);

	LOG_INF("Coil write, addr %u, %d", addr, (int)state);

	return 0;
}

static int holding_reg_rd(uint16_t addr, uint16_t *reg)
{
	if (addr >= SENSOR_COUNT) {
		return -ENOTSUP;
	}

	*reg = mb_mem.holding_registers[addr];

	LOG_INF("Holding register read, addr %u", addr);

	return 0;
}

static int holding_reg_wr(uint16_t addr, uint16_t reg)
{
	if (addr >= SENSOR_COUNT) {
		return -ENOTSUP;
	}

	mb_mem.holding_registers[addr] = reg;

	LOG_INF("Holding register write, addr %u", addr);

	return 0;
}

static struct modbus_user_callbacks mbs_cbs = {
	.coil_rd = coil_rd,
	.coil_wr = coil_wr,
	.holding_reg_rd = holding_reg_rd,
	.holding_reg_wr = holding_reg_wr,
};

const static struct modbus_iface_param server_param = {
	.mode = MODBUS_MODE_RTU,
	.server = {
		.user_cb = &mbs_cbs,
		.unit_id = 1,
	},
	.serial = {
		.baud = 115200,
		.parity = UART_CFG_PARITY_NONE,
	},
};

#define MODBUS_NODE DT_ALIAS(modbus_rtu)


int init_modbus_server(void)
{
	const char iface_name[] = {DEVICE_DT_NAME(MODBUS_NODE)};
	int iface;

	iface = modbus_iface_get_by_name(iface_name);

	if (iface < 0) {
		LOG_ERR("Failed to get iface index for %s", iface_name);
		return iface;
	}

	return modbus_init_server(iface, server_param);
}