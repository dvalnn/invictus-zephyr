#include "services/modbus.h"
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/modbus/modbus.h>
#include <zephyr/sys/util.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hydra_modbus, LOG_LEVEL_INF);

// Published channels
ZBUS_CHAN_DECLARE(chan_valves);

// Subscribed channels
ZBUS_CHAN_DECLARE(chan_temps, chan_pressures);

static void modbus_listener_cb(const struct zbus_channel *chan);

ZBUS_LISTENER_DEFINE(modbus_listener, modbus_listener_cb);
ZBUS_CHAN_ADD_OBS(chan_temps, modbus_listener, CONFIG_MODBUS_ZBUS_LISTENER_PRIO);
ZBUS_CHAN_ADD_OBS(chan_pressures, modbus_listener, CONFIG_MODBUS_ZBUS_LISTENER_PRIO);


#define MODBUS_THREAD_STACK_SIZE 1024 // TODO: make KConfig
#define MODBUS_THREAD_PRIORITY 5 // TODO: make KConfig

K_THREAD_STACK_DEFINE(modbus_thread_stack, MODBUS_THREAD_STACK_SIZE);
static struct k_thread modbus_thread_data;

static modbus_memory_t mb_mem = {0};

static void modbus_listener_cb(const struct zbus_channel *chan)
{
	if(chan == NULL) {
		return;
	}

	if (chan == &chan_pressures) {
		const press_msg_t *msg = zbus_chan_const_msg(chan);
		if (msg == NULL) {
			return;
		}
		mb_mem.holding_registers[PRESSURE_1] = msg->pressure1;
		mb_mem.holding_registers[PRESSURE_2] = msg->pressure2;
		mb_mem.holding_registers[PRESSURE_3] = msg->pressure3;
		LOG_DBG("Pressures updated in holding registers");
		return;
	}
	if (chan == &chan_temps) {
		const temps_msg_t *msg = zbus_chan_const_msg(chan);
		if (msg == NULL) {
			return;
		}
		mb_mem.holding_registers[THERMO_1] = msg->thermo1;
		mb_mem.holding_registers[THERMO_2] = msg->thermo2;
		mb_mem.holding_registers[THERMO_3] = msg->thermo3;
		LOG_DBG("Temps updated in holding registers");
		return;
	}
}

static int coil_rd(uint16_t addr, bool *state)
{
	if (addr >= VALVE_COUNT) {
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
	if (addr >= VALVE_COUNT) {
		return -ENOTSUP;
	}

	if (state == true) {
		mb_mem.coils[addr / 8] |= BIT(addr % 8);
		on = true;
	} else {
		mb_mem.coils[addr / 8] &= ~BIT(addr % 8);
		on = false;
	}
	// Maybe too much work to do in a callback?
	valves_msg_t msg;

	for (size_t i = 0; i < sizeof(mb_mem.coils); i++) {
		msg.valve_states[i] = mb_mem.coils[i];
	}

	zbus_chan_pub(&chan_valves, &msg, K_NO_WAIT);
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


int modbus_setup(void)
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

void modbus_thread_entry(void *p1, void *p2, void *p3) {
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_INF("Modbus RTU server thread starting");

	while (1) {
		k_sleep(K_MSEC(1000));
	}
}

void modbus_start(void) {
	k_tid_t tid = k_thread_create(&modbus_thread_data, modbus_thread_stack,
                                  K_THREAD_STACK_SIZEOF(modbus_thread_stack),
                                  modbus_thread_entry,
                                  NULL, NULL, NULL,
                                  MODBUS_THREAD_PRIORITY, 0, K_NO_WAIT);

    k_thread_name_set(tid, "modbus_thread");
}