#include "services/fake_lora.h"

ZBUS_CHAN_DECLARE(chan_radio_cmds);
LOG_MODULE_REGISTER(lora_backend_testing, LOG_LEVEL_DBG);

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
#define MSG_SIZE         32

K_MSGQ_DEFINE(uart_msgq, RADIO_CMD_SIZE, 10, 4);
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

static char rx_buf[RADIO_CMD_SIZE];
static int rx_buf_pos;

static void serial_cb(const struct device *dev, void *user_data)
{
    if (dev == NULL) {
        LOG_ERR("UART device is NULL");
        return;
    }

    if (!uart_irq_update(uart_dev)) {
        return;
    }

    int ret = uart_irq_rx_ready(uart_dev);
    if (ret <= 0) {
        if (ret == -ENOTSUP) {
            LOG_ERR("API not enabled\n");
        } else if (ret == -ENOSYS) {
            LOG_ERR("function not implemented\n");
        } else {
            LOG_INF("Rx not ready: %d\n", ret);
        }
        return;
    }

    // read a full command or until the buffer is full
    uint8_t c;
    while (uart_fifo_read(uart_dev, &c, 1) == 1) {
        if (rx_buf_pos == (RADIO_CMD_SIZE - 1)) {
            k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);
            rx_buf_pos = 0;
        } else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
            rx_buf[rx_buf_pos++] = c;
        }
        // else: characters beyond buffer size are dropped
    }
}

bool fake_lora_setup(void)
{
    if (uart_dev == NULL) {
        LOG_ERR("UART device not found!");
        return 0;
    }

    LOG_INF("UART device found: %s", uart_dev->name);
    if (!device_is_ready(uart_dev)) {
        LOG_ERR("UART device not found!");
        return 0;
    }
    LOG_INF("UART device is ready: %s", uart_dev->name);

    /* configure interrupt and callback to receive data */
    int ret = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);
    LOG_INF("UART callback set");
    if (ret < 0) {
        if (ret == -ENOTSUP) {
            LOG_ERR("Interrupt-driven UART API support not enabled\n");
        } else if (ret == -ENOSYS) {
            LOG_ERR("UART device does not support interrupt-driven API\n");
        } else {
            LOG_ERR("Error setting UART callback: %d\n", ret);
        }
        return 0;
    }
    return true;
}

void fake_lora_backend()
{
    LOG_INF("Fake LoRa backend (UART) started");
    uart_irq_rx_enable(uart_dev);
    struct generic_cmd_s cmd = {0};

    // indefinitely wait for input from the user
    while (k_msgq_get(&uart_msgq, &rx_buf, K_FOREVER) == 0) {
        if (sizeof(rx_buf) != CMD_SIZE) {
            LOG_ERR("Invalid command size: %d (expected %d)", sizeof(rx_buf), CMD_SIZE);
            LOG_HEXDUMP_WRN(rx_buf, sizeof(rx_buf), "Invalid command hex dump");
            continue;
        }

        int err = cmd_unpack((const uint8_t *const)rx_buf, sizeof(rx_buf), &cmd);
        if (err != PACK_ERROR_NONE) {
            LOG_ERR("Failed to unpack command: %d", err);
            LOG_HEXDUMP_WRN(rx_buf, sizeof(rx_buf), "Invalid command hex dump");
            continue;
        }

        int rc = zbus_chan_pub(&chan_radio_cmds, (const void *)&cmd, K_NO_WAIT);
        if (rc != 0) {
            LOG_ERR("Failed to publish command to channel: %d", rc);
            continue;
        }

        LOG_INF("Published command ID %d to cmd channel", cmd.header.command_id);
    }

    k_oops(); // unreachable
}
