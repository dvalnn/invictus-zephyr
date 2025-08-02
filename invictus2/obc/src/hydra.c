#include "hydra.h"

#include "zephyr/modbus/modbus.h"
#include "zephyr/logging/log.h"

#include "errno.h"

LOG_MODULE_REGISTER(hydra, LOG_LEVEL_INF);

inline void hydras_init(struct hydras *h)
{
    BUILD_ASSERT((CONFIG_HYDRA_UF_SLAVE_ID != CONFIG_HYDRA_LF_SLAVE_ID),
                 "UF and LF hydras must have different slave IDs.");
    BUILD_ASSERT((CONFIG_HYDRA_UF_SLAVE_ID > 0) && (CONFIG_HYDRA_LF_SLAVE_ID > 0),
                 "UF and LF hydras must have valid slave IDs greater than 0.");

    h->uf.slave_id = CONFIG_HYDRA_UF_SLAVE_ID;
    h->lf.slave_id = CONFIG_HYDRA_LF_SLAVE_ID;

    h->uf.is_connected = false;
    h->lf.is_connected = false;

    // Initialize solenoids to default states
    h->uf.solenoids.raw = 0;
    h->lf.solenoids.raw = 0;

    // Initialize sensors to default values
    h->lf.sensors.raw[0] = 0; // lf_temperature
    h->lf.sensors.raw[1] = 0; // lf_pressure
    h->lf.sensors.raw[2] = 0; // cc_pressure

    // Initialize temperatures
    h->uf.temperature = 0;
}

inline bool hydras_connected(const struct hydras *h)
{
    return h->uf.is_connected && h->lf.is_connected;
}

inline bool hydras_is_uf_connected(const struct hydras *h)
{
    return h->uf.is_connected;
}

inline bool hydras_is_lf_connected(const struct hydras *h)
{
    return h->lf.is_connected;
}

int hydras_modbus_read(struct hydras *h, const int client_iface)
{
    if (!h || client_iface < 0) {
        return -EINVAL;
    }

    // Read UF hydra
    const int uf_read = modbus_read_input_regs(
        client_iface, h->uf.slave_id, CONFIG_HYDRA_UF_INPUT_ADDR_START, &h->uf.temperature, 1);
    if (uf_read < 0) {
        LOG_ERR("Failed to read UF hydra: %d", uf_read);
    }

    // Read LF hydra
    const int lf_read =
        modbus_read_input_regs(client_iface, h->lf.slave_id, CONFIG_HYDRA_LF_INPUT_ADDR_START,
                               (uint16_t *)&h->lf.sensors.raw, ARRAY_SIZE(h->lf.sensors.raw));

    if (lf_read < 0) {
        LOG_ERR("Failed to read LF hydra: %d", lf_read);
    }

    return (uf_read < 0 || lf_read < 0) ? -EIO : 0;
}
