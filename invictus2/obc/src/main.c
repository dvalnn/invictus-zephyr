#include "zephyr/kernel.h"
#include "zephyr/kernel/thread.h"
#include "zephyr/logging/log.h"
#include "zephyr/devicetree.h"
#include "zephyr/usb/usb_device.h"
#include "zephyr/drivers/uart.h"

#include "filling_sm.h"
#include "modbus_thrd.h"

// THREADS:
// - Main thread: LoRa communication. (for now just pipe everything to stdout).
//   - Pre-Flight Mode: Switches between receive and transmit modes based
//   periodically -- High priority thread.
//   - Flight Model: passively sends data to the ground station. Should not
//   block other threads, switch to low priority.
// - Modbus thread: Reads holding registers from the slave devices and updates
// the filling state machine. -- Medium priority thread.
// - Navigator thread: Reads data from the navigator board via uart. -- Medium
// priority thread.
// - Data Thread: Saves sensor data and debug logs to the SD card. -- Low
// priority thread.
//
// LOGGING:
// - Log debugg information to the console and to a file on the SD card.
// // - Use LOG_INF, LOG_ERR, LOG_DBG, etc. macros for logging.

LOG_MODULE_REGISTER(obc, LOG_LEVEL_INF);

// Thread definitions
#define THREAD_STACK_SIZE      2048
// NOTE: lower values have higher priority
#define THREAD_PRIORITY_LOW    K_PRIO_PREEMPT(10)
#define THREAD_PRIORITY_MEDIUM K_PRIO_PREEMPT(5)
#define THREAD_PRIORITY_HIGH   K_PRIO_PREEMPT(1)

/* K_THREAD_STACK_DEFINE(lora_thread_stack, THREAD_STACK_SIZE); */
/* K_THREAD_STACK_DEFINE(navigator_thread_stack, THREAD_STACK_SIZE); */
/* K_THREAD_STACK_DEFINE(data_thread_stack, THREAD_STACK_SIZE); */

/* static struct k_thread lora_thread_data; */
/* static struct k_thread navigator_thread_data; */
/* static struct k_thread data_thread_data; */

K_SEM_DEFINE(modbus_start_sem, 0, 1);

K_MSGQ_DEFINE(fsm_cmd_q, sizeof(cmd_t), 2, 1);
K_MSGQ_DEFINE(modbus_sensor_data_q, sizeof(union filling_data), 2, 1);

DEFAULT_FSM_CONFIG(filling_sm_config);

static const struct modbus_data_queues mb_queues = {
    .fsm_cmd_q = &fsm_cmd_q,
    .sensor_data_q = &modbus_sensor_data_q,
};

K_THREAD_DEFINE(modbus_tid, THREAD_STACK_SIZE, modbus_thread_entry, &filling_sm_config,
                &mb_queues, &modbus_start_sem, THREAD_PRIORITY_MEDIUM, 0, 0);
extern const k_tid_t modbus_tid;

#define CDC_ACM_UART_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_cdc_acm_uart)

int main()
{

#if DT_NODE_EXISTS(CDC_ACM_UART_NODE)
    {
        const struct device *const dev = DEVICE_DT_GET(CDC_ACM_UART_NODE);
        uint32_t dtr = 0;

        if (!device_is_ready(dev) || usb_enable(NULL)) {
            return 0;
        }

        while (!dtr) {
            uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
            k_sleep(K_MSEC(100));
        }

        LOG_INF("CDC ACM UART connected on %s", dev->name);
    }
#endif

    LOG_INF("Starting OBC application");

    k_sem_give(&modbus_start_sem);
    k_thread_join(modbus_tid, K_FOREVER);
    k_oops(); // Should never reach here
}
