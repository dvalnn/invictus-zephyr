#ifndef ZBUS_VALIDATORS_H_
#define ZBUS_VALIDATORS_H_

#include <stddef.h>
#include <stdbool.h>

bool radio_cmd_validator(const void *msg, size_t msg_size);
bool rocket_state_validator(const void *msg, size_t msg_size);

#endif // ZBUS_VALIDATORS_H_
