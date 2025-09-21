#include "shell.h"
#include "packets.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>

LOG_MODULE_REGISTER(obc_shell, LOG_LEVEL_INF);

ZBUS_CHAN_DECLARE(chan_packets);

static int packet_abort_handler(const struct shell *sh, size_t argc, char **argv);
static int packet_ready_handler(const struct shell *sh, size_t argc, char **argv);
static int packet_arm_handler(const struct shell *sh, size_t argc, char **argv);
static int packet_fire_handler(const struct shell *sh, size_t argc, char **argv);
static int packet_launch_ovrd_handler(const struct shell *sh, size_t argc, char **argv);
static int packet_stop_handler(const struct shell *sh, size_t argc, char **argv);
static int packet_safe_pause_handler(const struct shell *sh, size_t argc, char **argv);
static int packet_resume_handler(const struct shell *sh, size_t argc, char **argv);
static int packet_manual_toggle_handler(const struct shell *sh, size_t argc, char **argv);

static int packet_fill_n2_handler(const struct shell *sh, size_t argc, char **argv);
static int packet_fill_pre_press_handler(const struct shell *sh, size_t argc, char **argv);
static int packet_fill_n2o_handler(const struct shell *sh, size_t argc, char **argv);
static int packet_fill_post_press_handler(const struct shell *sh, size_t argc, char **argv);

static int packet_manual_exec_handler(const struct shell *sh, size_t argc, char **argv);

SHELL_STATIC_SUBCMD_SET_CREATE(
    fill_exec_subcmd_set,

    SHELL_CMD_ARG(n2, NULL,
                  "fill n2 command. Params: <target_weight_grams> <trigger_temp_deci_c>",
                  packet_fill_n2_handler, 0, 2),

    SHELL_CMD_ARG(pre_press, NULL,
                  "pre-press command. Params: <target_tank_deci_bar> <trigger_tank_deci_bar>",
                  packet_fill_pre_press_handler, 0, 2),

    SHELL_CMD_ARG(n2o, NULL,
                  "fill n2o command. Params: <target_weight_grams> <trigger_temp_deci_c>",
                  packet_fill_n2o_handler, 0, 2),

    SHELL_CMD_ARG(press, NULL,
                  "post-press command. Params: <target_tank_deci_bar> <trigger_tank_deci_bar>",
                  packet_fill_post_press_handler, 0, 2),

    SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
    packet_subcmd_set,

    SHELL_CMD(abort, NULL, "Publish 'abort' command to zbus", packet_abort_handler),
    SHELL_CMD(ready, NULL, "Publish 'ready' command to zbus", packet_ready_handler),
    SHELL_CMD(arm, NULL, "Publish 'arm' command to zbus", packet_arm_handler),
    SHELL_CMD(fire, NULL, "Publish 'fire' command to zbus", packet_fire_handler),
    SHELL_CMD(launch_ovrd, NULL, "Publish 'launch override' command to zbus",
              packet_launch_ovrd_handler),
    SHELL_CMD(stop, NULL, "Publish 'stop' command to zbus", packet_stop_handler),
    SHELL_CMD(safe_pause, NULL, "Publish 'safe pause' command to zbus",
              packet_safe_pause_handler),
    SHELL_CMD(resume, NULL, "Publish 'resume' command to zbus", packet_resume_handler),
    SHELL_CMD(manual_toggle, NULL, "Publish 'manual toggle' command to zbus",
              packet_manual_toggle_handler),

    SHELL_CMD(fill_exec, &fill_exec_subcmd_set, "Publish 'fill exec' command to zbus", NULL),

    SHELL_CMD_ARG(manual_exec, NULL, "Publish 'manual exec' command to zbus",
                  packet_manual_exec_handler, 0, 2),

    SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(packet, &packet_subcmd_set, "Packet commands", NULL);

static int pub_cmd(const struct shell *sh, command_t cmd)
{
    shell_print(sh, "Publishing command %d", cmd);
    generic_packet_t pack = create_cmd_packet(cmd);
    shell_print(sh, "Packet created");
    int ret = zbus_chan_pub(&chan_packets, &pack, K_NO_WAIT);

    if (ret == 0)
    {
        shell_print(sh, "Published command %d", cmd);
        return 0;
    }
    else
    {
        shell_print(sh, "Failed to publish command %d: %d", cmd, ret);
        return -EIO;
    }
}

static int packet_abort_handler(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    return pub_cmd(sh, CMD_ABORT);
}

static int packet_ready_handler(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(sh);
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    return pub_cmd(sh, CMD_READY);
}

static int packet_arm_handler(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(sh);
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    return pub_cmd(sh, CMD_ARM);
}

static int packet_fire_handler(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(sh);
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    return pub_cmd(sh, CMD_FIRE);
}

static int packet_launch_ovrd_handler(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(sh);
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    return pub_cmd(sh, CMD_LAUNCH_OVERRIDE);
}

static int packet_stop_handler(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(sh);
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    return pub_cmd(sh, CMD_STOP);
}

static int packet_safe_pause_handler(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(sh);
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    return pub_cmd(sh, CMD_SAFE_PAUSE);
}

static int packet_resume_handler(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(sh);
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    return pub_cmd(sh, CMD_RESUME);
}

static int packet_manual_toggle_handler(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(sh);
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    return pub_cmd(sh, CMD_MANUAL_TOGGLE);
}

static int packet_fill_n2_handler(const struct shell *sh, size_t argc, char **argv)
{
    if (argc < 3)
    {
        shell_print(
            sh, "Usage: packet fill_exec fill_n2 <target_weight_grams> <trigger_temp_deci_c>");
        return -EINVAL;
    }

    int err = 0;
    struct fill_N2_params_s payload = {
        .target_N2_deci_bar = (uint16_t)shell_strtol(argv[1], 10, &err),
        .trigger_N2_deci_bar = (uint16_t)shell_strtol(argv[2], 10, &err),
    };

    if (err != 0)
    {
        shell_print(sh, "Invalid argument");
        return -EINVAL;
    }

    generic_packet_t generic_pack = create_cmd_packet(CMD_FILL_EXEC);
    struct cmd_fill_exec_s *const fill_exec = (struct cmd_fill_exec_s *)&generic_pack;
    fill_exec->payload.program_id = CMD_FILL_N2;
    memcpy(fill_exec->payload.params, &payload, sizeof(payload));

    return zbus_chan_pub(&chan_packets, &generic_pack, K_NO_WAIT);
}

static int packet_fill_pre_press_handler(const struct shell *sh, size_t argc, char **argv)
{
    if (argc < 3)
    {
        shell_print(sh, "Usage: packet fill_exec pre_press <target_tank_deci_bar> "
                        "<trigger_tank_deci_bar>");
        return -EINVAL;
    }

    int err = 0;
    struct fill_press_params_s payload = {
        .target_tank_deci_bar = (uint16_t)shell_strtol(argv[1], 10, &err),
        .trigger_tank_deci_bar = (uint16_t)shell_strtol(argv[2], 10, &err),
    };

    if (err != 0)
    {
        shell_print(sh, "Invalid argument");
        return -EINVAL;
    }

    generic_packet_t generic_pack = create_cmd_packet(CMD_FILL_EXEC);
    struct cmd_fill_exec_s *const fill_exec = (struct cmd_fill_exec_s *)&generic_pack;
    fill_exec->payload.program_id = CMD_FILL_PRE_PRESS;
    memcpy(fill_exec->payload.params, &payload, sizeof(payload));

    return zbus_chan_pub(&chan_packets, &generic_pack, K_NO_WAIT);
}

static int packet_fill_n2o_handler(const struct shell *sh, size_t argc, char **argv)
{
    if (argc < 3)
    {
        shell_print(
            sh,
            "Usage: packet fill_exec fill_n2o <target_weight_grams> <trigger_temp_deci_c>");
        return -EINVAL;
    }

    int err = 0;
    struct fill_N2O_params_s payload = {
        .target_weight_grams = (uint16_t)shell_strtol(argv[1], 10, &err),
        .trigger_temp_deci_c = (int16_t)shell_strtol(argv[2], 10, &err),
    };

    if (err != 0)
    {
        shell_print(sh, "Invalid argument");
        return -EINVAL;
    }

    generic_packet_t generic_pack = create_cmd_packet(CMD_FILL_EXEC);
    struct cmd_fill_exec_s *const fill_exec = (struct cmd_fill_exec_s *)&generic_pack;
    fill_exec->payload.program_id = CMD_FILL_N2O;
    memcpy(fill_exec->payload.params, &payload, sizeof(payload));

    return zbus_chan_pub(&chan_packets, &generic_pack, K_NO_WAIT);
}

static int packet_fill_post_press_handler(const struct shell *sh, size_t argc, char **argv)
{
    if (argc < 3)
    {
        shell_print(sh, "Usage: packet fill_exec post_press <target_tank_deci_bar> "
                        "<trigger_tank_deci_bar>");
        return -EINVAL;
    }

    int err = 0;
    struct fill_press_params_s payload = {
        .target_tank_deci_bar = (uint16_t)shell_strtol(argv[1], 10, &err),
        .trigger_tank_deci_bar = (uint16_t)shell_strtol(argv[2], 10, &err),
    };

    if (err != 0)
    {
        shell_print(sh, "Invalid argument");
        return -EINVAL;
    }

    generic_packet_t generic_pack = create_cmd_packet(CMD_FILL_EXEC);
    struct cmd_fill_exec_s *const fill_exec = (struct cmd_fill_exec_s *)&generic_pack;
    fill_exec->payload.program_id = CMD_FILL_POST_PRESS;
    memcpy(fill_exec->payload.params, &payload, sizeof(payload));

    return zbus_chan_pub(&chan_packets, &generic_pack, K_NO_WAIT);
}

static int packet_manual_exec_handler(const struct shell *sh, size_t argc, char **argv)
{
    LOG_WRN(" 'packet_manual' is not implemented");
    return 0;
}

void setup_shell_if_enabled(void)
{
#if DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_shell_uart), zephyr_cdc_acm_uart)
    const struct device *dev;
    uint32_t dtr = 0;

    LOG_INF("Initializing cdc acm uart backend");
    dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));
    if (!device_is_ready(dev))
    {
        LOG_ERR("UART device not ready");
        k_oops();
    }

    LOG_INF("Waiting uart DTR");
    while (!dtr)
    {
        uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
        k_sleep(K_MSEC(100));
    }
#endif
}
