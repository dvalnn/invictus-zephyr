#include <invictus2/lib/modbus/common.h>

static struct modbus_server *active_server = NULL;

#define K_MALLOC_NON_ZERO(count, size) ((count) > 0 ? k_malloc((count) * (size)) : NULL)
#define K_FREE_NOT_NULL(ptr)           (ptr ? k_free(ptr) : (void)0)

#define REG_OP_BEGIN(server, meta, addr, regptr)                                              \
    do                                                                                        \
    {                                                                                         \
        if (!(server) || !(regptr))                                                           \
            return -EINVAL;                                                                   \
        if ((addr) < (meta).start_addr || (addr) >= (meta).start_addr + (meta).count)         \
            return -EINVAL;                                                                   \
        if (k_mutex_lock(&(server)->regs_mutex,                                               \
                         K_MSEC(CONFIG_MODBUS_REG_MTX_LOCK_TIMEOUT)) != 0)                    \
            return -EBUSY;                                                                    \
    } while (0)

#define REG_OP_END(server) k_mutex_unlock(&(server)->regs_mutex)

static int coil_rd(uint16_t addr, bool *state);
static int coil_wr(uint16_t addr, bool state);
static int discrete_input_rd(uint16_t addr, bool *state);
static int input_reg_rd(uint16_t addr, uint16_t *reg);
static int input_reg_rd_fp(uint16_t addr, float *reg);
static int holding_reg_rd(uint16_t addr, uint16_t *reg);
static int holding_reg_wr(uint16_t addr, uint16_t reg);
static int holding_reg_rd_fp(uint16_t addr, float *reg);
static int holding_reg_wr_fp(uint16_t addr, float reg);

struct modbus_user_callbacks user_cbs = {
    .coil_rd = coil_rd,
    .coil_wr = coil_wr,
    .discrete_input_rd = discrete_input_rd,
    .input_reg_rd = input_reg_rd,
#if defined(CONFIG_MODBUS_FP_EXTENSIONS)
    .input_reg_rd_fp = input_reg_rd_fp,
#endif
    .holding_reg_rd = holding_reg_rd,
    .holding_reg_wr = holding_reg_wr,
#if defined(CONFIG_MODBUS_FP_EXTENSIONS)
    .holding_reg_rd_fp = holding_reg_rd_fp,
    .holding_reg_wr_fp = holding_reg_wr_fp,
#endif
};

int modbus_server_init(struct modbus_server *server, struct modbus_server_meta meta)
{
    if (!server)
    {
        return -EINVAL;
    }

    server->meta = meta;

    server->regs.coils = K_MALLOC_NON_ZERO(meta.coils.count, sizeof(uint8_t));
    server->regs.discrete_inputs =
        K_MALLOC_NON_ZERO(meta.discrete_inputs.count, sizeof(uint8_t));
    server->regs.holding_registers =
        K_MALLOC_NON_ZERO(meta.holding_registers.count, sizeof(uint16_t));
    server->regs.input_registers =
        K_MALLOC_NON_ZERO(meta.input_registers.count, sizeof(uint16_t));

#if defined(CONFIG_MODBUS_FP_EXTENSIONS)
    server->regs.input_registers_fp =
        K_MALLOC_NON_ZERO(meta.input_registers_fp.count, sizeof(float));
    server->regs.holding_registers_fp =
        K_MALLOC_NON_ZERO(meta.holding_registers_fp.count, sizeof(float));
#endif

    return k_mutex_init(&server->regs_mutex);
}

int modbus_server_free(struct modbus_server *server)
{
    if (!server)
    {
        return -EINVAL;
    }

    if (active_server == server)
    {
        return -EBUSY;
    }

    K_FREE_NOT_NULL(server->regs.coils);
    K_FREE_NOT_NULL(server->regs.discrete_inputs);
    K_FREE_NOT_NULL(server->regs.holding_registers);
    K_FREE_NOT_NULL(server->regs.input_registers);

#if defined(CONFIG_MODBUS_FP_EXTENSIONS)
    K_FREE_NOT_NULL(server->regs.input_registers_fp);
    K_FREE_NOT_NULL(server->regs.holding_registers_fp);
#endif

    return 0;
}

int modbus_server_start(const struct modbus_server *const server, int modbus_iface,
                        struct modbus_serial_param serial_param)
{
    if (!server)
    {
        return -EINVAL;
    }

    if (active_server)
    {
        return -EBUSY;
    }

    struct modbus_iface_param iface_param = {
        .mode = MODBUS_MODE_RTU,
        .serial = serial_param,
        .server =
            {
                .user_cb = &user_cbs,
                .unit_id = server->meta.unit_id,
            },
    };

    active_server = (struct modbus_server *)server;
    return modbus_init_server(server->meta.unit_id, iface_param);
}

int modbus_server_stop(const struct modbus_server *const server)
{
    if (!server || active_server != server)
    {
        return -EINVAL;
    }

    active_server = NULL;
    return modbus_disable(server->meta.unit_id);
}

static int coil_rd(uint16_t addr, bool *state)
{
    REG_OP_BEGIN(active_server, active_server->meta.coils, addr, active_server->regs.coils);

    uint8_t *coils = active_server->regs.coils;
    uint16_t start = active_server->meta.coils.start_addr;

    uint8_t bit = (addr - start) % 8;
    size_t offset = (addr - start) / 8;

    *state = (coils[offset] >> (bit)) & 0x01;

    return REG_OP_END(active_server);
}

static int coil_wr(uint16_t addr, bool state)
{
    REG_OP_BEGIN(active_server, active_server->meta.coils, addr, active_server->regs.coils);

    uint8_t *coils = active_server->regs.coils;
    uint16_t start = active_server->meta.coils.start_addr;

    uint8_t bit = (addr - start) % 8;
    size_t offset = (addr - start) / 8;

    state ? (coils[offset] |= (1 << bit)) : (coils[offset] &= ~(1 << bit));

    return REG_OP_END(active_server);
}

static int discrete_input_rd(uint16_t addr, bool *state)
{
    REG_OP_BEGIN(active_server, active_server->meta.discrete_inputs, addr,
                 active_server->regs.discrete_inputs);

    uint16_t start = active_server->meta.discrete_inputs.start_addr;
    uint8_t *dinputs = active_server->regs.discrete_inputs;

    uint8_t bit = (addr - start) % 8;
    size_t offset = (addr - start) / 8;

    *state = (dinputs[offset] >> bit) & 0x01;

    return REG_OP_END(active_server);
}

static int input_reg_rd(uint16_t addr, uint16_t *reg)
{
    REG_OP_BEGIN(active_server, active_server->meta.input_registers, addr,
                 active_server->regs.input_registers);

    uint16_t start = active_server->meta.input_registers.start_addr;
    *reg = active_server->regs.input_registers[addr - start];

    return REG_OP_END(active_server);
}

static int input_reg_rd_fp(uint16_t addr, float *reg)
{
    REG_OP_BEGIN(active_server, active_server->meta.input_registers_fp, addr,
                 active_server->regs.input_registers_fp);

    uint16_t start = active_server->meta.input_registers_fp.start_addr;
    *reg = active_server->regs.input_registers_fp[addr - start];

    return REG_OP_END(active_server);
}

static int holding_reg_rd(uint16_t addr, uint16_t *reg)
{
    REG_OP_BEGIN(active_server, active_server->meta.holding_registers, addr,
                 active_server->regs.holding_registers);

    uint16_t start = active_server->meta.holding_registers.start_addr;
    *reg = active_server->regs.holding_registers[addr - start];

    return REG_OP_END(active_server);
}

static int holding_reg_wr(uint16_t addr, uint16_t reg)
{
    REG_OP_BEGIN(active_server, active_server->meta.holding_registers, addr,
                 active_server->regs.holding_registers);

    uint16_t start = active_server->meta.holding_registers.start_addr;
    active_server->regs.holding_registers[addr - start] = reg;

    return REG_OP_END(active_server);
}

static int holding_reg_rd_fp(uint16_t addr, float *reg)
{
    REG_OP_BEGIN(active_server, active_server->meta.holding_registers_fp, addr,
                 active_server->regs.holding_registers_fp);

    uint16_t start = active_server->meta.holding_registers_fp.start_addr;
    *reg = active_server->regs.holding_registers_fp[addr - start];

    return REG_OP_END(active_server);
}

static int holding_reg_wr_fp(uint16_t addr, float reg)
{
    REG_OP_BEGIN(active_server, active_server->meta.holding_registers, addr,
                 active_server->regs.holding_registers_fp);

    uint16_t start = active_server->meta.holding_registers_fp.start_addr;
    active_server->regs.holding_registers_fp[addr - start] = reg;

    return REG_OP_END(active_server);
}
