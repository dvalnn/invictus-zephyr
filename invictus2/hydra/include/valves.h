// Simple valve control API for HYDRA
// - Solenoid valves are toggled via GPIO (from board DTS node labels)
// - A servo valve helper (PWM angle) remains available for compatibility

#include <stdbool.h>
#include <stdint.h>

#include "data_models.h"
#include "peripherals/pwm.h"

// Initialize all valve GPIOs as outputs, default closed (inactive)
int valves_init(void);

// Start the valves workqueue
void valves_start(void);

// Set a valve state: open = true to energize coil, false to close
int valve_set(valve_t id, bool open);

// Open a valve for a duration in milliseconds, then auto-close
int valve_open_ms(valve_t id, uint32_t ms);

// Query cached state
bool valve_is_open(valve_t id);

// PWM servo valve control 
void valve_pwm_set_angle(float angle);