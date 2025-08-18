#include "services/modbus/internal/hydra.h"
#include "services/modbus/modbus.h"

#include "zephyr/modbus/modbus.h"
#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(hydra, LOG_LEVEL_INF);

inline void hydra_boards_init(struct hydra_boards *const hb)
{
    BUILD_ASSERT((CONFIG_MODBUS_HYDRA_UF_SLAVE_ID > 0) &&
                     (CONFIG_MODBUS_HYDRA_LF_SLAVE_ID > 0) &&
                     (CONFIG_MODBUS_HYDRA_FS_SLAVE_ID > 0) &&
                     (CONFIG_MODBUS_HYDRA_UF_SLAVE_ID != CONFIG_MODBUS_HYDRA_LF_SLAVE_ID !=
                      CONFIG_MODBUS_HYDRA_FS_SLAVE_ID),
                 "all hydra boards must have different, non-zero, slave IDs.");

    if (!hb) {
        LOG_ERR("Hydras boards structure pointer is NULL.");
        return;
    }

    memset(hb, 0, sizeof(struct hydra_boards)); // REVIEW: if *h is always statically
                                                // allocated, this is not needed
    hb->uf.meta.slave_id = CONFIG_MODBUS_HYDRA_UF_SLAVE_ID;
    hb->uf.meta.ir_start = CONFIG_MODBUS_HYDRA_UF_INPUT_ADDR_START;

    hb->lf.meta.slave_id = CONFIG_MODBUS_HYDRA_LF_SLAVE_ID;
    hb->lf.meta.ir_start = CONFIG_MODBUS_HYDRA_LF_INPUT_ADDR_START;

    hb->fs.meta.slave_id = CONFIG_MODBUS_HYDRA_FS_SLAVE_ID;
    hb->fs.meta.ir_start = CONFIG_MODBUS_HYDRA_FS_INPUT_ADDR_START;
}

void hydra_boards_sensor_read(const int client_iface, struct hydra_boards *const h)
{
    if (!h || client_iface < 0) {
        LOG_ERR("Invalid parameters for hydra sensor read.");
        return;
    }

    // Read upper feed hydra sensors
    int uf_rc = modbus_read_input_regs(client_iface, h->uf.meta.slave_id, h->uf.meta.ir_start,
                                       (uint16_t *const)&h->uf.sensors.raw,
                                       ARRAY_SIZE(h->uf.sensors.raw));

    int lf_rc = modbus_read_input_regs(client_iface, h->lf.meta.slave_id, h->lf.meta.ir_start,
                                       (uint16_t *const)&h->lf.sensors.raw,
                                       ARRAY_SIZE(h->lf.sensors.raw));

    // TODO: flag to skip filling station sensors read after quick disconnect / launch
    int fs_rc = modbus_read_input_regs(client_iface, h->fs.meta.slave_id, h->fs.meta.ir_start,
                                       (uint16_t *const)&h->fs.sensors.raw,
                                       ARRAY_SIZE(h->fs.sensors.raw));

    modbus_slave_check_connection(uf_rc, &h->uf.meta, "UF hydra sensors");
    modbus_slave_check_connection(lf_rc, &h->lf.meta, "LF hydra sensors");
    modbus_slave_check_connection(fs_rc, &h->fs.meta, "FS hydra sensors");
}

void hydra_boards_irs_to_zbus_rep(const struct hydra_boards *const hb,
                                  union thermocouples_u *const thermocouples,
                                  union pressures_u *const pressures)
{
    if (!hb || !thermocouples || !pressures) {
        LOG_ERR("Invalid parameters for hydra IRs to ZBUS conversion.");
        return;
    }

    // Initialize the thermocouples union
    thermocouples->n2o_tank_uf_t1 = hb->uf.sensors.uf_temperature1;
    thermocouples->n2o_tank_uf_t2 = hb->uf.sensors.uf_temperature2;
    thermocouples->n2o_tank_uf_t3 = hb->uf.sensors.uf_temperature3;

    thermocouples->n2o_tank_lf_t1 = hb->lf.sensors.lf_temperature1;
    thermocouples->n2o_tank_lf_t2 = hb->lf.sensors.lf_temperature2;

    thermocouples->chamber_thermo = 0; // REVIEW: MISSING FROM hydra.h

    thermocouples->n2o_line_thermo1 = hb->fs.sensors.n2o_temperature1; // before solenoid
    thermocouples->n2o_line_thermo2 = hb->fs.sensors.n2o_temperature2; // after solenoid
    thermocouples->n2_line_thermo = hb->fs.sensors.n2_temperature;

    // Initialize the pressures union
    /* pressures->n2o_tank_pressure = hb->uf.sensors.n2o_pressure; */
}
