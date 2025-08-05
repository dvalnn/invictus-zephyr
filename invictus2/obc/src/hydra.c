#include "hydra.h"

#include "zephyr/modbus/modbus.h"
#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(hydra, LOG_LEVEL_INF);

inline void rocket_hydras_init(struct rocket_hydras *h)
{
    BUILD_ASSERT((CONFIG_HYDRA_UF_SLAVE_ID > 0) && (CONFIG_HYDRA_LF_SLAVE_ID > 0) &&
                     (CONFIG_HYDRA_UF_SLAVE_ID != CONFIG_HYDRA_LF_SLAVE_ID),
                 "UF and LF hydras must have different, non-zero, slave IDs.");

    if (!h) {
        LOG_ERR("Hydras structure pointer is NULL.");
        return;
    }

    memset(h, 0, sizeof(struct rocket_hydras)); // REVIEW: if *h is always statically
                                                // allocated, this is not needed
    h->uf.slave_id = CONFIG_HYDRA_UF_SLAVE_ID;
    h->lf.slave_id = CONFIG_HYDRA_LF_SLAVE_ID;
}

void hydras_sensor_read(const int client_iface, struct hydra_sensor_read_desc *const batch,
                        const uint8_t n_reads)
{
    if (!batch || n_reads == 0 || client_iface < 0) {
        LOG_ERR("Invalid parameters for sensor read.");
        return;
    }

    for (uint8_t i = 0; i < n_reads; i++) {
        struct hydra_sensor_read_desc *read = &batch[i];
        const int ret = modbus_read_input_regs(client_iface, read->slave_id, read->start_addr,
                                               read->dest, read->len);
        if (ret < 0) {
            LOG_ERR("Failed to read [%s]: %d", read->label, ret);
            if (read->on_error) {
                read->on_error(read->label, ret);
            }
        }
    }
}

static inline int modbus_read_coils_u8(const char *const label, const int client_iface,
                                       const uint8_t slave_id, const uint16_t addr,
                                       uint8_t *const dest, const uint16_t len)
{
    int rc = modbus_read_coils(client_iface, slave_id, addr, dest, len);
    if (rc < 0) {
        LOG_ERR("Read coils failed [%s]: %d", label, rc);
    }
    return rc;
}
