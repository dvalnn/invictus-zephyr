// Simple valve control API for HYDRA
// - Solenoid valves are toggled via GPIO (from board DTS node labels)
// - A servo valve helper (PWM angle) remains available for compatibility

#include <stdbool.h>
#include <stdint.h>

#include "pwm.h"

// Logical IDs for available HYDRA valves (as per inv2_hydra_rp2040.dts)
typedef enum {
	HYDRA_VALVE_SOL1 = 0,     // sol_valve_1
	HYDRA_VALVE_SOL2,         // sol_valve_2
	HYDRA_VALVE_SOL3,         // sol_valve_3
	HYDRA_VALVE_QDC_N2O,      // qdc_n2o
	HYDRA_VALVE_QDC_N2,       // qdc_n2
	HYDRA_VALVE_STEEL1,       // st_valve_1
	HYDRA_VALVE_STEEL2,       // st_valve_2
	HYDRA_VALVE_COUNT
} hydra_valve_t;

// Initialize all valve GPIOs as outputs, default closed (inactive)
int valves_init(void);

// Set a valve state: open = true to energize coil, false to close
int valve_set(hydra_valve_t id, bool open);

// Open a valve for a duration in milliseconds, then auto-close
int valve_open_ms(hydra_valve_t id, uint32_t ms);

// Query cached state
bool valve_is_open(hydra_valve_t id);

// PWM servo valve control 
void valve_pwm_set_angle(float angle);

// Configure a given DAC channel
bool valve_dac_configure(hydra_valve_t id, uint16_t value);
