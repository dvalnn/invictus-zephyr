#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/modbus/modbus.h>
#include <zephyr/random/random.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hydra, LOG_LEVEL_INF);

/* Configuration */
#define SENSOR1_PERIOD_MS 1000  /* Temperature sensor period */
#define SENSOR2_PERIOD_MS 2000  /* Pressure sensor period */
#define VALVE_CHECK_PERIOD_MS 500

/* Modbus register map */
#define REG_TEMP     0
#define REG_PRESSURE 1
#define REG_VALVES   2

/* Thread stack sizes */
#define STACK_SIZE 1024
#define THREAD_PRIORITY 7

static int client_iface;

/* Shared data structures */
static struct {
    uint16_t temperature;
    uint16_t pressure;
    uint8_t valve_states;
} system_data;

const static struct modbus_iface_param client_param = {
	.mode = MODBUS_MODE_RTU,
	.rx_timeout = 50000,
	.serial = {
		.baud = 19200,
		.parity = UART_CFG_PARITY_NONE,
	},
};

#define MODBUS_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_modbus_serial)

/* Thread function prototypes */
static void temperature_sensor_thread(void *, void *, void *);
static void pressure_sensor_thread(void *, void *, void *);
static void valve_control_thread(void *, void *, void *);

/* Thread definitions */
K_THREAD_DEFINE(temp_thread_id, STACK_SIZE,
                temperature_sensor_thread, NULL, NULL, NULL,
                THREAD_PRIORITY, 0, 0);

K_THREAD_DEFINE(pressure_thread_id, STACK_SIZE,
                pressure_sensor_thread, NULL, NULL, NULL,
                THREAD_PRIORITY, 0, 0);

K_THREAD_DEFINE(valve_thread_id, STACK_SIZE,
                valve_control_thread, NULL, NULL, NULL,
                THREAD_PRIORITY, 0, 0);

static int init_modbus_client(void)
{
	const char iface_name[] = {DEVICE_DT_NAME(MODBUS_NODE)};
	client_iface = modbus_iface_get_by_name(iface_name);
	return modbus_init_client(client_iface, client_param);
}

/* Simulated sensor reading (replace with actual sensor code) */
static uint16_t read_temperature(void) {
    return 20 + (k_uptime_get_32() % 10);  // Simulated value between 20-30
}

static uint16_t read_pressure(void) {
    return 1000 + (k_uptime_get_32() % 100);  // Simulated value between 1000-1100
}

/* Thread implementations */
static void temperature_sensor_thread(void *p1, void *p2, void *p3)
{
    while (1) {
        system_data.temperature = read_temperature();
        
        /* Write temperature to Modbus holding register */
        int err = modbus_write_holding_regs(client_iface, 1, REG_TEMP, 
                                          &system_data.temperature, 1);
        if (err) {
            LOG_ERR("Failed to write temperature to Modbus: %d", err);
        }
        
        k_msleep(SENSOR1_PERIOD_MS);
    }
}

static void pressure_sensor_thread(void *p1, void *p2, void *p3)
{
    while (1) {
        system_data.pressure = read_pressure();
        
        /* Write pressure to Modbus holding register */
        int err = modbus_write_holding_regs(client_iface, 1, REG_PRESSURE,
                                          &system_data.pressure, 1);
        if (err) {
            LOG_ERR("Failed to write pressure to Modbus: %d", err);
        }
        
        k_msleep(SENSOR2_PERIOD_MS);
    }
}

static void valve_control_thread(void *p1, void *p2, void *p3)
{
    uint16_t valve_command;
    
    while (1) {
        /* Read valve commands from Modbus */
        int err = modbus_read_holding_regs(client_iface, 1, REG_VALVES,
                                         &valve_command, 1);
        if (err == 0) {
            system_data.valve_states = valve_command & 0xFF;
            
            /* Update physical valves based on received commands */
            /* Replace with actual valve control code */
            LOG_INF("Setting valves state to: 0x%02x", system_data.valve_states);
        } else {
            LOG_ERR("Failed to read valve commands: %d", err);
        }
        
        k_msleep(VALVE_CHECK_PERIOD_MS);
    }
}

int main(void)
{
    /* Initialize Modbus client */
    if (init_modbus_client()) {
        LOG_ERR("Modbus RTU client initialization failed");
        return 0;
    }

    LOG_INF("Hydra system initialized");
    LOG_INF("- Temperature sampling: %d ms", SENSOR1_PERIOD_MS);
    LOG_INF("- Pressure sampling: %d ms", SENSOR2_PERIOD_MS);
    LOG_INF("- Valve control period: %d ms", VALVE_CHECK_PERIOD_MS);

    /* Threads are automatically started by K_THREAD_DEFINE */
    while (1) {
        k_sleep(K_FOREVER);  /* Let the threads do the work */
    }

    return 0;
}