#include "radio_commands.h"
#include "services/lora.h"
#include "invictus2/drivers/sx128x_context.h"
#include "radio_commands.h"

#include "syscalls/kernel.h"
#include "zephyr/kernel/thread_stack.h"
#include "zephyr/logging/log.h"
#include "zephyr/sys/ring_buffer.h"
#include "zephyr/toolchain.h"
#include "zephyr/kernel.h"
#include "zephyr/zbus/zbus.h"
#include <stdint.h>

LOG_MODULE_REGISTER(lora_integration, LOG_LEVEL_DBG);
ZBUS_CHAN_DECLARE(chan_radio_cmds);

static lora_context_t *ctx = NULL;
// TODO fine tune size
static uint8_t lora_rx_buffer[253 * 5];

static void lora_on_recv_data(uint8_t *payload, uint16_t size)
{
    // 1. copy payload to ringbuffer
    ctx->rx_size += ring_buf_put(&ctx->rx_rb, payload, size);

    // 2. trigger semaphore
    k_sem_give(&ctx->data_available);
}

bool lora_service_setup(lora_context_t *context)
{
    LOG_INF("Setting up LoRa thread...");

    if (context == NULL)
    {
        LOG_ERR("Invalid context object");
        return false;
    }

    if (context->stop_signal == NULL)
    {
        LOG_ERR("Invalid stop source");
        return false;
    }

    ctx = context;

    const struct device *dev = DEVICE_DT_GET(DT_ALIAS(lora0));
    if (dev == NULL)
    {
        LOG_ERR("failed to find loRa device");
        return false;
    }

    k_sem_init(&ctx->data_available, 0, 1);
    ring_buf_init(&ctx->rx_rb, sizeof(lora_rx_buffer), lora_rx_buffer);
    ctx->rx_size = 0;

    sx128x_register_recv_callback(&lora_on_recv_data);
    LOG_INF("initialized loRa service thread");
    return true;
}

void lora_handle_incoming_packet()
{
    if (ctx == NULL)
    {
        LOG_ERR("Invalid context");
        k_oops();
    }

    if (ctx->rx_size == 0)
    {
        LOG_DBG("No packet to process");
        return;
    }

    // TODO handle _all_ incoming packets
    uint8_t *raw_msg = NULL;
    ring_buf_get_claim(&ctx->rx_rb, &raw_msg, sizeof(struct radio_generic_cmd_s));

    struct radio_generic_cmd_s *msg = (struct radio_generic_cmd_s *)raw_msg;
    zbus_chan_pub(&chan_radio_cmds, (const void *)msg, K_NO_WAIT);
    ring_buf_get_finish(&ctx->rx_rb, sizeof(struct radio_generic_cmd_s));
}

void lora_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    if (ctx == NULL)
    {
        LOG_ERR("Lora service has not been properly configured");
        return;
    }

    LOG_INF("LoRa thread starting");
    const uint32_t c_sleep_time_ms = 80;
    while (*ctx->stop_signal != 1)
    {
        LOG_INF("LoRa thread");

        // handle lora reception
        if (k_sem_take(&ctx->data_available, K_NO_WAIT) == 0)
        {
            LOG_INF("read %u bytes", ctx->rx_size);
            lora_handle_incoming_packet();
        }

        // handle zbus reception

        // TODO sleep only difference
        k_sleep(K_MSEC(c_sleep_time_ms));
    }

    // unregister callback
    sx128x_register_recv_callback(NULL);
    LOG_INF("LoRa thread exiting.");
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
