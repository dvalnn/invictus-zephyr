#include "services/fake_lora.h"

ZBUS_CHAN_DECLARE(chan_packets);
LOG_MODULE_REGISTER(lora_backend_testing, LOG_LEVEL_DBG);

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
#define MSG_SIZE         PACKET_SIZE

K_MSGQ_DEFINE(uart_msgq, PACKET_SIZE, 10, 4);
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

static char rx_buf[PACKET_SIZE];
static int rx_buf_pos = 0;

static void serial_cb(const struct device *dev, void *user_data)
{
    if (dev == NULL) {
        LOG_ERR("UART device is NULL");
        return;
    }

    if (!uart_irq_update(uart_dev)) {
        LOG_ERR("Didn't update uart irq");
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
    //uint8_t c[32];
    //rx_buf_pos = 0;
    /*
    while (uart_fifo_read(uart_dev, &c, 1) == 1) {
        //LOG_INF("Fifo read");
        if (rx_buf_pos == (PACKET_SIZE - 1)) {
            LOG_INF("Sent message to queue");
            k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);
            rx_buf_pos = 0;
        } else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
            LOG_INF("%d: %d", rx_buf_pos, c);
            rx_buf[rx_buf_pos++] = c;
        } else {
            LOG_INF("Dropped byte");
        }
    }
    */
   int bytes_read = 0;
   bytes_read += uart_fifo_read(uart_dev, &rx_buf[rx_buf_pos], 32);
   rx_buf_pos += bytes_read;
   if(rx_buf_pos == (PACKET_SIZE)) {
    k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);
    rx_buf_pos = 0;
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
    LOG_INF("Enabled UART RX IRQ");
    
    struct generic_packet_s packet = {0};

    // indefinitely wait for input from the user
    while (k_msgq_get(&uart_msgq, &rx_buf, K_FOREVER) == 0) {
        LOG_INF("Received something");

        if (sizeof(rx_buf) != PACKET_SIZE) {
            LOG_ERR("Invalid command size: %d (expected %d)", sizeof(rx_buf), PACKET_SIZE);
            LOG_HEXDUMP_WRN(rx_buf, sizeof(rx_buf), "Invalid command hex dump");
            continue;
        }

        int err = packet_unpack((const uint8_t *const)rx_buf, sizeof(rx_buf), &packet);
        if (err != PACK_ERROR_NONE) {
            LOG_ERR("Failed to unpack command: %d", err);
            LOG_HEXDUMP_WRN(rx_buf, sizeof(rx_buf), "Invalid command hex dump");
            continue;
        }

        int rc = zbus_chan_pub(&chan_packets, (const void *)&packet, K_NO_WAIT);
        if (rc != 0) {
            LOG_ERR("Failed to publish packet to channel: %d", rc);
            continue;
        }

        LOG_INF("Published packet with cmd ID %d to packet channel", packet.header.command_id);
    }

    k_oops(); // unreachable
}
