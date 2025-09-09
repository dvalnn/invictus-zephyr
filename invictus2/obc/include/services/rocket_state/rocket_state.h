#ifndef ROCKET_STATE_H_
#define ROCKET_STATE_H_

#include <stdbool.h>

bool rocket_state_service_setup(void);
void rocket_state_service_start(void);

// State Machine Work Helper
void toggle_valve(int valve_id, bool open);

#endif // ROCKET_STATE_H_
