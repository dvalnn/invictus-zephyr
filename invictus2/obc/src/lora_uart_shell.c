#include "lora_uart_shell.h"

#include "zephyr/toolchain.h"
#include "zephyr/logging/log.h"
#include "zephyr/kernel.h"
#include "zephyr/device.h"
#include "zephyr/drivers/uart.h"

LOG_MODULE_REGISTER(lora_thread_uart, LOG_LEVEL_DBG);

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
#define MSG_SIZE         32

K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

static char rx_buf[MSG_SIZE];
static int rx_buf_pos;

/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
void serial_cb(const struct device *dev, void *user_data)
{
    uint8_t c;

    if (!uart_irq_update(uart_dev)) {
        return;
    }

    if (!uart_irq_rx_ready(uart_dev)) {
        return;
    }

    /* read until FIFO empty */
    while (uart_fifo_read(uart_dev, &c, 1) == 1) {
        if ((c == '\n' || c == '\r') && rx_buf_pos > 0) {
            /* terminate string */
            rx_buf[rx_buf_pos] = '\0';

            /* if queue is full, message is silently dropped */
            k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);

            /* reset the buffer (it was copied to the msgq) */
            rx_buf_pos = 0;
        } else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
            rx_buf[rx_buf_pos++] = c;
        }
        /* else: characters beyond buffer size are dropped */
    }
}

bool lora_uart_setup(void)
{
    if (!device_is_ready(uart_dev)) {
        LOG_ERR("UART device not found!");
        return false;
    }

    /* configure interrupt and callback to receive data */
    int ret = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);

    if (ret < 0) {
        if (ret == -ENOTSUP) {
            LOG_ERR("Interrupt-driven UART API support not enabled\n");
        } else if (ret == -ENOSYS) {
            LOG_ERR("UART device does not support interrupt-driven API\n");
        } else {
            LOG_ERR("Error setting UART callback: %d\n", ret);
        }
        return false;
    }

    return true;
}

void lora_uart_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    char tx_buf[MSG_SIZE];

    uart_irq_rx_enable(uart_dev);

    LOG_INF("LORA UART Shell thread started");

    /* indefinitely wait for input from the user */
    while (k_msgq_get(&uart_msgq, &tx_buf, K_FOREVER) == 0) {
        LOG_DBG("ECHO: %s", tx_buf);

        // TODO: redirect received data to msgq to process lora commands
    }

    k_oops(); // unreachable code, but added to avoid compiler warnings
}
