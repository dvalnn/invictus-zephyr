#include "lift.h"

#include "zephyr/kernel.h"
#include "zephyr/modbus/modbus.h"
#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(lift, LOG_LEVEL_INF);

inline void rocket_lift_init(struct r_lift *l)
{
    BUILD_ASSERT((CONFIG_LIFT_R_SLAVE_ID > 0) && (CONFIG_LIFT_FS_SLAVE_ID > 0) &&
                     (CONFIG_LIFT_R_SLAVE_ID != CONFIG_LIFT_FS_SLAVE_ID),
                 "R and FS lifts must have different, non-zero, slave IDs.");

    if (!l) {
        LOG_ERR("Lift structure pointer is NULL.");
        return;
    }

    memset(l, 0, sizeof(struct r_lift)); // REVIEW: if *h is always statically
                                            // allocated, this is not needed
    l->meta.slave_id = CONFIG_LIFT_R_SLAVE_ID;
    l->meta.ir_start = CONFIG_LIFT_R_INPUT_ADDR_START;
}

inline void fs_lift_init(struct fs_lift *l)
{
    BUILD_ASSERT((CONFIG_LIFT_R_SLAVE_ID > 0) && (CONFIG_LIFT_FS_SLAVE_ID > 0) &&
                 (CONFIG_LIFT_R_SLAVE_ID != CONFIG_LIFT_FS_SLAVE_ID),
             "R and FS lifts must have different, non-zero, slave IDs.");

    if (!l) {
        LOG_ERR("Filling Station Lift structure pointer is NULL.");
        return;
    }

    memset(l, 0, sizeof(struct fs_lift)); // REVIEW: if *h is always statically
                                            // allocated, this is not needed
    l->meta.slave_id = CONFIG_LIFT_FS_SLAVE_ID;
    l->meta.ir_start = CONFIG_LIFT_FS_INPUT_ADDR_START;
}

static inline void check_connection(const int rc, struct lift_metadata *meta,
                                    const char *const label)
{
    if (!meta || !label) {
        LOG_ERR("Invalid parameters for connection check.");
        return;
    }

    if (rc < 0 && !meta->is_connected) {
        return; // Already disconnected, no need to log again
    }

    if (rc < 0 && meta->is_connected) {
        LOG_ERR("Failed to read [%s]: %d. Flagging disconnect.", label, rc);
        meta->is_connected = false;
    } else if (rc >= 0 && !meta->is_connected) {
        LOG_INF("Reconnected to [%s].", label);
        meta->is_connected = true;
    }
}

void rocket_lift_sensor_read(const int client_iface, struct r_lift *const l)
{
    if (!l || client_iface < 0) {
        LOG_ERR("Invalid parameters for LIFT sensor read.");
        return;
    }

    // Read upper feed lift sensors
    int lift_rc = modbus_read_input_regs(client_iface, l->meta.slave_id, l->meta.ir_start,
                                       (uint16_t *const)&l->loadcells.raw,
                                       ARRAY_SIZE(l->loadcells.raw));

    check_connection(lift_rc, &l->meta, "Lift sensors");
}

void rocket_lift_coils_read(const int client_iface, struct r_lift *const l)
{
    k_oops(); // FIXME: implement this function
}

void fs_lift_sensor_read(const int client_iface, struct fs_lift *const l)
{
    if (!l || client_iface < 0) {
        LOG_ERR("Invalid parameters for FS LIFT sensor read.");
        return;
    }

    // Read filling station lift sensors
    int lift_rc = modbus_read_input_regs(client_iface, l->meta.slave_id, l->meta.ir_start,
                                       (uint16_t *const)&l->n2o_loadcell,
                                       1); // Only one loadcell

    check_connection(lift_rc, &l->meta, "FS Lift sensors");
}

void fs_lift_coils_read(const int client_iface, struct fs_lift *const l)
{
    k_oops(); // FIXME: implement this function
}