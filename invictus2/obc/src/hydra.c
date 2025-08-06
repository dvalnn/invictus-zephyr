#include "hydra.h"

#include "zephyr/kernel.h"
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
    h->uf.meta.slave_id = CONFIG_HYDRA_UF_SLAVE_ID;
    h->uf.meta.ir_start = CONFIG_HYDRA_UF_INPUT_ADDR_START;

    h->lf.meta.slave_id = CONFIG_HYDRA_LF_SLAVE_ID;
    h->lf.meta.ir_start = CONFIG_HYDRA_LF_INPUT_ADDR_START;
}

static inline void check_connection(const int rc, struct hydra_metadata *meta,
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

void rocket_hydras_sensor_read(const int client_iface, struct rocket_hydras *const h)
{
    if (!h || client_iface < 0) {
        LOG_ERR("Invalid parameters for hydra sensor read.");
        return;
    }

    // Read upper feed hydra sensors
    int uf_rc = modbus_read_input_regs(client_iface, h->uf.meta.slave_id, h->uf.meta.ir_start,
                                       &h->uf.temperature, 1);
    int lf_rc = modbus_read_input_regs(client_iface, h->lf.meta.slave_id, h->lf.meta.ir_start,
                                       (uint16_t *const)&h->lf.sensors.raw,
                                       ARRAY_SIZE(h->lf.sensors.raw));

    check_connection(uf_rc, &h->uf.meta, "UF hydra sensors");
    check_connection(lf_rc, &h->lf.meta, "LF hydra sensors");
}

void rocket_hydras_coils_read(const int client_iface, struct rocket_hydras *const h)
{
    k_oops(); // FIXME: implement this function
}
