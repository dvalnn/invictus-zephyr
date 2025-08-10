#include "lift.h"

#include "services/modbus.h"

#include "zephyr/kernel.h"
#include "zephyr/modbus/modbus.h"
#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(lift, LOG_LEVEL_INF);

inline void lift_init(struct rocket_lift *r, struct fs_lift *l)
{
    BUILD_ASSERT((CONFIG_MODBUS_ROCKET_LIFT_SLAVE_ID > 0) &&
                     (CONFIG_MODBUS_FS_LIFT_SLAVE_ID > 0) &&
                     (CONFIG_MODBUS_ROCKET_LIFT_SLAVE_ID != CONFIG_MODBUS_FS_LIFT_SLAVE_ID),
                 "R and FS lifts must have different, non-zero, slave IDs.");

    if (!r || !l) {
        LOG_ERR("Lift structure pointer is NULL.");
        return;
    }

    memset(r, 0, sizeof(struct rocket_lift)); // REVIEW: if *r is always statically
    memset(l, 0, sizeof(struct rocket_lift)); // REVIEW: if *l is always statically

    // allocated, this is not needed
    r->meta.slave_id = CONFIG_MODBUS_ROCKET_LIFT_SLAVE_ID;
    r->meta.ir_start = CONFIG_MODBUS_ROCKET_LIFT_INPUT_ADDR_START;
    l->meta.slave_id = CONFIG_MODBUS_FS_LIFT_SLAVE_ID;
    l->meta.ir_start = CONFIG_MODBUS_FS_LIFT_INPUT_ADDR_START;
}

void rocket_lift_sensor_read(const int client_iface, struct rocket_lift *const l)
{
    if (!l || client_iface < 0) {
        LOG_ERR("Invalid parameters for LIFT sensor read.");
        return;
    }

    // Read upper feed lift sensors
    int read_res = modbus_read_input_regs(client_iface, l->meta.slave_id, l->meta.ir_start,
                                          (uint16_t *const)&l->loadcells.raw,
                                          ARRAY_SIZE(l->loadcells.raw));

    modbus_slave_check_connection(read_res, &l->meta, "Rocket LIFT");
}

void rocket_lift_coils_read(const int client_iface, struct rocket_lift *const l)
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
    int read_res = modbus_read_input_regs(client_iface, l->meta.slave_id, l->meta.ir_start,
                                          (uint16_t *const)&l->n2o_loadcell,
                                          1); // Only one loadcell

    modbus_slave_check_connection(read_res, &l->meta, "FS LIFT");
}

void fs_lift_coils_read(const int client_iface, struct fs_lift *const l)
{
    k_oops(); // FIXME: implement this function
}
