#include "services/modbus.h"
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/modbus/modbus.h>
#include <zephyr/sys/util.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hydra_modbus, LOG_LEVEL_INF);

#define MODBUS_NODE DT_ALIAS(modbus_rtu)

// Published channels
ZBUS_CHAN_DECLARE(chan_valves);

// Subscribed channels
ZBUS_CHAN_DECLARE(chan_temps, chan_pressures);

static void modbus_listener_cb(const struct zbus_channel *chan);

ZBUS_LISTENER_DEFINE(modbus_listener, modbus_listener_cb);
ZBUS_CHAN_ADD_OBS(chan_temps, modbus_listener, CONFIG_MODBUS_ZBUS_LISTENER_PRIO);
ZBUS_CHAN_ADD_OBS(chan_pressures, modbus_listener, CONFIG_MODBUS_ZBUS_LISTENER_PRIO);

K_THREAD_STACK_DEFINE(modbus_work_q_stack, CONFIG_MODBUS_WORK_Q_STACK);
static struct k_work_q modbus_work_q;

static void mb_temps_work_handler(struct k_work *work);
static void mb_press_work_handler(struct k_work *work);
static void mb_valves_work_handler(struct k_work *work);

static K_WORK_DEFINE(modbus_temps_work, mb_temps_work_handler);
static K_WORK_DEFINE(modbus_press_work, mb_press_work_handler);
static K_WORK_DEFINE(modbus_valves_work, mb_valves_work_handler);

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
		k_work_submit_to_queue(&modbus_work_q, &modbus_press_work);
		return;
	}
	if (chan == &chan_temps) {
		const temps_msg_t *msg = zbus_chan_const_msg(chan);
		if (msg == NULL) {
			return;
		}
		k_work_submit_to_queue(&modbus_work_q, &modbus_temps_work);
		return;
	}
}

static void mb_temps_work_handler(struct k_work *work)
{
    temps_msg_t temps;
    zbus_chan_read(&chan_temps, &temps, K_NO_WAIT);

	mb_mem.holding_registers[THERMO_1] = temps.thermo1;
	mb_mem.holding_registers[THERMO_2] = temps.thermo2;
	mb_mem.holding_registers[THERMO_3] = temps.thermo3;
}

static void mb_press_work_handler(struct k_work *work)
{
    press_msg_t pressures;
    zbus_chan_read(&chan_pressures, &pressures, K_NO_WAIT);

	mb_mem.holding_registers[PRESSURE_1] = pressures.pressure1;
	mb_mem.holding_registers[PRESSURE_2] = pressures.pressure2;
	mb_mem.holding_registers[PRESSURE_3] = pressures.pressure3;
}

static void mb_valves_work_handler(struct k_work *work)
{
	valves_msg_t msg;

	for (size_t i = 0; i < sizeof(mb_mem.coils); i++) {
		msg.valve_states[i] = mb_mem.coils[i];
	}

	zbus_chan_pub(&chan_valves, &msg, K_NO_WAIT);
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
	k_work_submit_to_queue(&modbus_work_q, &modbus_valves_work);
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

void modbus_start(void) {
	k_work_queue_start(&modbus_work_q, modbus_work_q_stack,
                       K_THREAD_STACK_SIZEOF(modbus_work_q_stack), CONFIG_MODBUS_WORK_Q_PRIO,
                       NULL);

}