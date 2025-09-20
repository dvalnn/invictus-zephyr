// HYDRA GPIO abstraction layer
// Encapsulates Zephyr GPIO control for valves so higher layers don't touch
// low-level APIs directly.

#ifndef HYDRA_GPIO_H_
#define HYDRA_GPIO_H_

#include <stdbool.h>

// Initialize all HYDRA valve GPIOs as outputs, default inactive (closed)
// Returns 0 on success, <0 on error (last error if multiple)
int gpio_init_valves();

// Set valve GPIO by logical index (0..N-1) to active (open=true) or inactive
// Returns 0 on success, <0 on error
int gpio_set_valve(int id, bool open);

#endif // HYDRA_GPIO_H_

