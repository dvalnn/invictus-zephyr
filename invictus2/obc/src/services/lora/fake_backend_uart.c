#include "services/fake_lora.h"

LOG_MODULE_REGISTER(lora_backend_testing, LOG_LEVEL_DBG);
#include "radio_commands.h"

extern struct zbus_channel chan_radio_cmds;

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
#define MSG_SIZE         32

K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

static char rx_buf[MSG_SIZE];
static int rx_buf_pos;

int d = 0;
static void serial_cb(const struct device *dev, void *user_data)
{
    if(dev == NULL) {
        LOG_ERR("UART device is NULL");
        return;
    }

    //k_sleep(K_MSEC(1)); // debounce
    LOG_INF("UART interrupt received, d=%d", d++);
    uint8_t c;

	if (!uart_irq_update(uart_dev)) {
		return;
	}

	if (!uart_irq_rx_ready(uart_dev)) {
		return;
	}

	// read until FIFO empty 
	while (uart_fifo_read(uart_dev, &c, 1) == 1) {
		if ((c == '\n' || c == '\r') && rx_buf_pos > 0) {
			// terminate string 
			rx_buf[rx_buf_pos] = '\0';

			// if queue is full, message is silently dropped 
			k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);

			// reset the buffer (it was copied to the msgq) 
			rx_buf_pos = 0;
		} else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
			rx_buf[rx_buf_pos++] = c;
		}
		// else: characters beyond buffer size are dropped 
	}
}

/*
 * Print a null-terminated string character by character to the UART interface
 */
void print_uart(char *buf)
{
	int msg_len = strlen(buf);

	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart_dev, buf[i]);
	}
}

bool fake_lora_setup(void)
{
    if(uart_dev == NULL) {
        LOG_ERR("UART device not found!");
        return 0;
    }
    LOG_INF("UART device found: %s", uart_dev->name);
    if (!device_is_ready(uart_dev)) {
		LOG_ERR("UART device not found!");
		return 0;
	}
    LOG_INF("UART device is ready: %s", uart_dev->name);

    serial_cb(NULL, NULL); // just to test if callback works

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
	print_uart("Hello! I'm your echo bot.\r\n");
	print_uart("Tell me something and press enter:\r\n");

	// indefinitely wait for input from the user 
	while (k_msgq_get(&uart_msgq, &rx_buf, K_FOREVER) == 0) {
		print_uart("Echo: ");
		print_uart(rx_buf);
		print_uart("\r\n");
        LOG_ERR("Received command: %s", rx_buf);
	}
            
    k_oops(); // unreachable
    
}
