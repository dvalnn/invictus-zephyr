#include "zbus_messages.h"
#include "zephyr/toolchain.h"

bool modbus_coil_write_msg_validator(const void *msg, size_t msg_size)
{
    ARG_UNUSED(msg_size);

    const struct modbus_write_coils_msg *m = msg;
    return (m != NULL) && (m->slave_id > 0) && (m->start_addr >= 0) && (m->num_coils > 0) &&
           (m->values != NULL);
}

bool rocket_event_msg_validator(const void *msg, size_t msg_size)
{
    ARG_UNUSED(msg_size);

    const struct rocket_event_msg *m = msg;
    return (m != NULL) && (m->event >= ROCKET_EVENT_NONE) && (m->event < _ROCKET_EVENT_COUNT);
}
