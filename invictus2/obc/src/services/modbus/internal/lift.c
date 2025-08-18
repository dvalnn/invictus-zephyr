#include "services/modbus/internal/lift.h"

#include "services/modbus/modbus.h"

#include "zephyr/modbus/modbus.h"
#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(lift, LOG_LEVEL_INF);

inline void lift_boards_init(struct lift_boards *const lb)
{
    BUILD_ASSERT((CONFIG_MODBUS_ROCKET_LIFT_SLAVE_ID > 0) &&
                     (CONFIG_MODBUS_FS_LIFT_SLAVE_ID > 0) &&
                     (CONFIG_MODBUS_ROCKET_LIFT_SLAVE_ID != CONFIG_MODBUS_FS_LIFT_SLAVE_ID),
                 "R and FS lifts must have different, non-zero, slave IDs.");

    if (!lb) {
        LOG_ERR("Lift structure pointer is NULL.");
        return;
    }

    memset(lb, 0, sizeof(struct lift_boards)); // REVIEW: if *r is always statically

    // allocated, this is not needed
    lb->rocket.meta.slave_id = CONFIG_MODBUS_ROCKET_LIFT_SLAVE_ID;
    lb->rocket.meta.ir_start = CONFIG_MODBUS_ROCKET_LIFT_INPUT_ADDR_START;

    lb->fs.meta.slave_id = CONFIG_MODBUS_FS_LIFT_SLAVE_ID;
    lb->fs.meta.ir_start = CONFIG_MODBUS_FS_LIFT_INPUT_ADDR_START;
}

void lift_boards_read_irs(const int client_iface, struct lift_boards *const lb)
{

    if (!lb || client_iface < 0) {
        LOG_ERR("Invalid parameters for LIFT sensor read.");
        return;
    }

    // Read upper feed lift sensors
    int rocket_rc = modbus_read_input_regs(
        client_iface, lb->rocket.meta.slave_id, lb->rocket.meta.ir_start,
        (uint16_t *const)&lb->rocket.loadcells.raw, ARRAY_SIZE(lb->rocket.loadcells.raw));

    // TODO: flag to skip filling station read after quick disconnect / launch
    int fs_rc =
        modbus_read_input_regs(client_iface, lb->fs.meta.slave_id, lb->fs.meta.ir_start,
                               (uint16_t *const)&lb->fs.n2o_loadcell, 1); // REVIEW:

    modbus_slave_check_connection(rocket_rc, &lb->rocket.meta, "Rocket LIFT");
    modbus_slave_check_connection(fs_rc, &lb->fs.meta, "Filling Station LIFT");
}

inline void lift_boards_irs_to_zbus_rep(const struct lift_boards *const lb,
                                        union loadcell_weights_u *const weights)
{
    if (!lb || !weights) {
        LOG_ERR("Invalid parameters for LIFT IRs ZBUS conversion.");
        return;
    }

    weights->n2o_loadcell = lb->fs.n2o_loadcell;
    weights->rail_loadcell = lb->rocket.loadcells.rail;

    // TODO: include based on KConfig option
    // Loadcells for static tests
    weights->thrust_loadcell1 = lb->rocket.loadcells.thrust_1;
    weights->thrust_loadcell2 = lb->rocket.loadcells.thrust_2;
    weights->thrust_loadcell3 = lb->rocket.loadcells.thrust_3;
}
