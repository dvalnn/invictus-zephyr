#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/toolchain.h>
#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>

#include "services/lora.h"
#include "services/fake_lora.h"
#include "invictus2/drivers/sx128x_context.h"
#include "data_models.h"
#include "packets.h"
#include <stdint.h>

LOG_MODULE_REGISTER(lora_thread, LOG_LEVEL_DBG);



#if !CONFIG_LORA_REDIRECT_UART
static lora_context_t *ctx = NULL;
// TODO fine tune size
static uint8_t lora_rx_buffer[253 * 5];

static void lora_on_recv_data(uint8_t *payload, uint16_t size)
{
    // 1. copy payload to ringbuffer
    ctx->rx_size = ring_buf_get_claim(&ctx->rx_rb, &payload, size);

    // 2. trigger semaphore
    k_sem_give(&ctx->data_available);
}
#endif

bool lora_service_setup(lora_context_t *context)
{
    LOG_INF("Setting up LoRa thread...");
    #if CONFIG_LORA_REDIRECT_UART
        LOG_INF("Setting up fake LoRa backend (UART)...");
        return fake_lora_setup();
    #else
        LOG_INF("Setting up real LoRa backend (SX128x)...");
        ctx = context;
        const struct device *dev = DEVICE_DT_GET(DT_ALIAS(lora0));
        if (dev == NULL) {
            LOG_ERR("failed to find loRa device");
            return false;
        }

        k_sem_init(&ctx->data_available, 0, 1);
        ring_buf_init(&ctx->rx_rb, sizeof(lora_rx_buffer), lora_rx_buffer);

        sx128x_register_recv_callback(&lora_on_recv_data);
        return true;
    #endif
    return true;
}

void lora_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    #if CONFIG_LORA_REDIRECT_UART
        LOG_INF("Starting fake LoRa backend (UART)...");
        fake_lora_backend();

        LOG_INF("Fake LoRa backend (UART) exiting.");
    #else
        if (ctx == NULL) {
            LOG_ERR("Lora service has not been properly configured");
            return;
        }
        LOG_INF("LoRa thread starting");
        const uint32_t c_sleep_time_ms = 80;
        while (*ctx->stop_signal != 1) {
            LOG_INF("LoRa thread");

            // handle lora reception
            if (k_sem_take(&ctx->data_available, K_MSEC(c_sleep_time_ms)) == 0) {
                LOG_INF("read %u bytes", ctx->rx_size);
            } else {
                LOG_DBG("LoRa timeout");
            }

            // handle zbus reception

            // TODO sleep only difference
            k_sleep(K_MSEC(c_sleep_time_ms));
        }

        LOG_INF("LoRa thread exiting.");
    #endif
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

/// @brief Builds a status response packet with the given system data 
/// @param rep pointer to the status_rep struct to be filled
void build_status_rep(struct cmd_status_rep_s * rep, system_data_t * data) {
    rep->hdr.command_id = CMD_STATUS_REP;
    rep->hdr.packet_version = SUPPORTED_PACKET_VERSION;
    rep->hdr.sender_id  = OBC_PACKET_ID;
    rep->hdr.target_id = GROUND_STATION_PACKET_ID;

    rep->payload = *data;
}