#include "shell.h"
#include "zephyr/shell/shell.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(obc_shell, LOG_LEVEL_INF);

static int packet_abort_handler(const struct shell *sh, size_t argc, char **argv);
static int packet_ready_handler(const struct shell *sh, size_t argc, char **argv);
static int packet_arm_handler(const struct shell *sh, size_t argc, char **argv);
static int packet_fire_handler(const struct shell *sh, size_t argc, char **argv);
static int packet_launch_ovrd_handler(const struct shell *sh, size_t argc, char **argv);
static int packet_stop_handler(const struct shell *sh, size_t argc, char **argv);
static int packet_safe_pause_handler(const struct shell *sh, size_t argc, char **argv);
static int packet_resume_handler(const struct shell *sh, size_t argc, char **argv);
static int packet_manual_toggle_handler(const struct shell *sh, size_t argc, char **argv);

static int packet_fill_exec_handler(const struct shell *sh, size_t argc, char **argv);
static int packet_manual_exec_handler(const struct shell *sh, size_t argc, char **argv);

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

    SHELL_CMD_ARG(fill_exec, NULL, "Publish 'fill exec' command to zbus",
                  packet_fill_exec_handler, 0, 2),
    SHELL_CMD_ARG(manual_exec, NULL, "Publish 'manual exec' command to zbus",
                  packet_manual_exec_handler, 0, 2),

    SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(packet, &packet_subcmd_set, "Packet commands", NULL);

static int packet_abort_handler(const struct shell *sh, size_t argc, char **argv)
{
    LOG_WRN(" 'packet_abort' is not implemented");
    return 0;
}

static int packet_ready_handler(const struct shell *sh, size_t argc, char **argv)
{
    LOG_WRN(" 'packet_ready' is not implemented");
    return 0;
}

static int packet_arm_handler(const struct shell *sh, size_t argc, char **argv)
{
    LOG_WRN(" 'packet_arm' is not implemented");
    return 0;
}

static int packet_fire_handler(const struct shell *sh, size_t argc, char **argv)
{
    LOG_WRN(" 'packet_fire' is not implemented");
    return 0;
}

static int packet_launch_ovrd_handler(const struct shell *sh, size_t argc, char **argv)
{
    LOG_WRN(" 'packet_launch' is not implemented");
    return 0;
}

static int packet_stop_handler(const struct shell *sh, size_t argc, char **argv)
{
    LOG_WRN(" 'packet_stop' is not implemented");
    return 0;
}

static int packet_safe_pause_handler(const struct shell *sh, size_t argc, char **argv)
{
    LOG_WRN(" 'packet_safe' is not implemented");
    return 0;
}

static int packet_resume_handler(const struct shell *sh, size_t argc, char **argv)
{
    LOG_WRN(" 'packet_resume' is not implemented");
    return 0;
}

static int packet_manual_toggle_handler(const struct shell *sh, size_t argc, char **argv)
{
    LOG_WRN(" 'packet_manual' is not implemented");
    return 0;
}

static int packet_fill_exec_handler(const struct shell *sh, size_t argc, char **argv)
{
    LOG_WRN(" 'packet_fill' is not implemented");
    return 0;
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
