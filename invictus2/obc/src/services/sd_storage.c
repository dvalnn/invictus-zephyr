#include "services/sd_storage.h"

#include "zbus_messages.h"

#include "zephyr/kernel.h"
#include "zephyr/zbus/zbus.h"
#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(sd_storage, LOG_LEVEL_INF);

// Channel declarations
ZBUS_CHAN_DECLARE(uf_hydra_chan);
ZBUS_CHAN_DECLARE(lf_hydra_chan);

static void uf_work_handler(struct k_work *work);
static void lf_work_handler(struct k_work *work);

static K_WORK_DEFINE(uf_work, uf_work_handler);
static K_WORK_DEFINE(lf_work, lf_work_handler);

// Shared work queue
K_THREAD_STACK_DEFINE(sd_worker_stack, 1024);
static struct k_work_q sd_worker_q;

// Listener callback
static void sd_listener_cb(const struct zbus_channel *chan)
{
    if (chan == &uf_hydra_chan) {
        k_work_submit_to_queue(&sd_worker_q, &uf_work);
    } else if (chan == &lf_hydra_chan) {
        k_work_submit_to_queue(&sd_worker_q, &lf_work);
    }
}

// Listener definition
ZBUS_LISTENER_DEFINE(sd_listener, sd_listener_cb);

// Register listener to channels
ZBUS_CHAN_ADD_OBS(uf_hydra_chan, sd_listener, SD_LISTENER_PRIO);
ZBUS_CHAN_ADD_OBS(lf_hydra_chan, sd_listener, SD_LISTENER_PRIO);

// Work handlers (called in work queue context)
static void uf_work_handler(struct k_work *work)
{
    static struct uf_hydra_msg msg;
    if (zbus_chan_read(&uf_hydra_chan, &msg, K_NO_WAIT) == 0) {
        LOG_INF("UF Hydra: Temperature: %d", msg.temperature);
    } else {
        LOG_WRN("UF read failed or empty");
    }
}

static void lf_work_handler(struct k_work *work)
{
    static struct lf_hydra_msg msg;
    if (zbus_chan_read(&lf_hydra_chan, &msg, K_NO_WAIT) == 0) {
        LOG_INF("LF Hydra: Temperature: %d, Pressure: %d, CC Pressure: %d", msg.lf_temperature,
                msg.lf_pressure, msg.cc_pressure);
    } else {
        LOG_WRN("LF read failed or empty");
    }
}

// Initialization code (call from `main()` or system init)
void start_sd_worker_q(void)
{
    k_work_queue_start(&sd_worker_q, sd_worker_stack, K_THREAD_STACK_SIZEOF(sd_worker_stack),
                       SD_WORKER_PRIO, NULL);
}
