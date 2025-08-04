#include "services/lora.h"

#include "filling_sm.h"

#include "zbus_messages.h"
#include "zephyr/kernel/thread_stack.h"
#include "zephyr/logging/log.h"
#include "zephyr/toolchain.h"
#include "zephyr/kernel.h"
#include "zephyr/device.h"
#include "zephyr/zbus/zbus.h"

LOG_MODULE_REGISTER(lora_thread, LOG_LEVEL_DBG);

ZBUS_CHAN_DECLARE(lora_cmd_chan);

#if CONFIG_LORA_REDIRECT_UART

#include "zephyr/drivers/uart.h"

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
#define MSG_SIZE         32

K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

static char rx_buf[MSG_SIZE];
static int rx_buf_pos;

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

void lora_uart_backend()
{
    char tx_buf[MSG_SIZE];

    uart_irq_rx_enable(uart_dev);

    LOG_INF("LORA UART Shell thread started");

    /* indefinitely wait for input from the user */
    while (k_msgq_get(&uart_msgq, &tx_buf, K_FOREVER) == 0) {
        LOG_DBG("CMD: %s", tx_buf);

        // split the tx_buf by spaces.
        // first string element is the command type tag
        // second element is the command integer tag
        //
        // Example:
        //  `normal 0x1` -> normal commands will be processed by the FSM
        //  `override 0x2 ...` -> override commands will be ignored for now

#define CMD_TYPE_TAG_SIZE 15

        char command_type[CMD_TYPE_TAG_SIZE + 1] = {0};
        cmd_t command = 0;

        int num_args = sscanf(tx_buf, "%15s %x", command_type, &command);
        if (num_args != 2) {
            LOG_ERR("Invalid command format: %s", tx_buf);
            continue;
        }

        // TODO: refactor
        if (strcmp(command_type, "normal") == 0) {
            LOG_INF("Processing normal command: %d", command);

            struct lora_cmd_msg cmd_msg = {
                .subsystem = SUBSYSTEM_ID_FSM,
                .command = command,
            };

            // TODO: better timeout handling
            int ret = zbus_chan_pub(&lora_cmd_chan, &cmd_msg, K_NO_WAIT);
            if (ret < 0) {
                LOG_ERR("Failed to publish command to LORA channel: %d", ret);
            }

        } else {
            LOG_WRN("Unsupported command type: %s", command_type);
        }
    }

    k_oops(); // unreachable
}

#else // NOT CONFIG_LORA_REDIRECT_UART

void lora_backend(void)
{
    // TODO

    LOG_INF("LoRa thread running mock logic...");
    k_sleep(K_SECONDS(3)); // Simulate work
    LOG_INF("LoRa thread exiting.");
}

#endif // CONFIG_LORA_REDIRECT_UART

bool lora_service_setup(void)
{
    LOG_INF("Setting up LoRa thread...");

#if CONFIG_LORA_REDIRECT_UART
    return lora_uart_setup();
#else
    // TODO
    return true; // No setup needed for non-UART LoRa backend
#endif // CONFIG_LORA_REDIRECT_UART
}

void lora_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

#if CONFIG_LORA_REDIRECT_UART
    return lora_uart_backend();
#else
    return lora_backend();
#endif // CONFIG_LORA_REDIRECT_UART
}

#define LORA_THREAD_PRIO 5               // TODO: make KConfig
K_THREAD_STACK_DEFINE(lora_stack, 1024); // TODO: make KConfig
static struct k_thread lora_thread;

void lora_service_start(void)
{
    const k_tid_t tid =
        k_thread_create(&lora_thread, lora_stack, K_THREAD_STACK_SIZEOF(lora_stack),
                        lora_thread_entry, NULL, NULL, NULL, LORA_THREAD_PRIO, 0, K_NO_WAIT);

    k_thread_name_set(tid, "lora");
}
