#include "zephyr/toolchain.h"

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>

LOG_MODULE_REGISTER(app);

static int cmd_gs_reset_usb_boot(const struct shell *sh, size_t argc, char **argv);
SHELL_CMD_ARG_REGISTER(reboot, NULL, "Reboot board into USB MSD boot mode.",
                       cmd_gs_reset_usb_boot, 1, 0);

#ifdef CONFIG_SOC_FAMILY_RPI_PICO
#include <pico/bootrom.h>

static int cmd_gs_reset_usb_boot(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(sh, "Rebooting into MSD boot mode.");
    shell_print(sh, "Mount partition and upload UF2 file.");

    reset_usb_boot(0, 0);
    return 0;
}

#else

static int cmd_gs_reset_usb_boot(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(sh);
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    LOG_ERROR("Reset into USB boot unsuported");
    return 0;
}

#endif

int main(void)
{
#if DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_shell_uart), zephyr_cdc_acm_uart)
    const struct device *dev;
    uint32_t dtr = 0;

    dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));
    if (!device_is_ready(dev)) {
        k_oops();
    }

    while (!dtr) {
        uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
        k_sleep(K_MSEC(100));
    }
#endif
    return 0;
}
