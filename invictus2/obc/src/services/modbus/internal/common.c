#include "services/modbus/internal/common.h"

#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(obc_modbus_common, LOG_LEVEL_DBG);

inline void modbus_slave_check_connection(const int read_result,
                                          struct modbus_slave_metadata *const meta,
                                          const char *const label)
{
    if (!meta || !label) {
        LOG_ERR("Invalid parameters for connection check.");
        return;
    }

    if (read_result < 0 && !meta->is_connected) {
        return; // Already disconnected, no need to log again
    }

    if (read_result < 0 && meta->is_connected) {
        LOG_ERR("Failed to read [%s]: %d. Flagging disconnect.", label, read_result);
        meta->is_connected = false;
    } else if (read_result >= 0 && !meta->is_connected) {
        LOG_INF("Reconnected to [%s].", label);
        meta->is_connected = true;
    }
}
