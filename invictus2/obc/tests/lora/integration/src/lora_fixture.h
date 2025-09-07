#ifndef OBC_TESTS_LORA_H
#define OBC_TESTS_LORA_H

#include <stdlib.h>
#include <stdbool.h>
#include "radio_commands.h"
#include "zephyr/kernel.h"
#include "zephyr/sys/atomic.h"
#include "zephyr/ztest_assert.h"
#include <invictus2/drivers/sx128x_context.h>
#include "services/lora.h"
#include "zephyr/zbus/zbus.h"

struct lora_integration_test_fixture
{
    atomic_t stop_signal;
    lora_context_t ctx;
    struct radio_generic_cmd_s received_messages[100];
    size_t number_of_received_messages;
};

// --- Radio Commands from Ground Station ---
ZBUS_CHAN_DEFINE(chan_radio_cmds,            /* Channel Name */
                 struct radio_generic_cmd_s, /* Message Type */
                 NULL,                       /* Validator Func */
                 NULL,                       /* User Data */
                 ZBUS_OBSERVERS_EMPTY,       /* Observers */
                 ZBUS_MSG_INIT(0)            /* Initial Value */
)

// FIXME this should be in a proper header file
extern bool fake_sx128x_is_rx_callback_set();
extern void fake_sx128x_rf_reception(uint8_t *, size_t);

// Hack to access fixture on callbacks
static struct lora_integration_test_fixture *g_fixture = NULL;

static void on_new_message(const struct zbus_channel *chan)
{
    if (g_fixture == NULL)
    {
        return;
    }

    if (g_fixture->number_of_received_messages >= sizeof(g_fixture->received_messages))
    {
        return;
    }

    g_fixture->received_messages[g_fixture->number_of_received_messages] =
        *(struct radio_generic_cmd_s *)zbus_chan_msg(&chan_radio_cmds);
    g_fixture->number_of_received_messages++;
}

ZBUS_LISTENER_DEFINE(radio_command_listener, on_new_message);
ZBUS_CHAN_ADD_OBS(chan_radio_cmds, radio_command_listener, 4);

static void *test_fixture_setup(void)
{
    zassert_is_null(g_fixture);
    struct lora_integration_test_fixture *fixture =
        malloc(sizeof(struct lora_integration_test_fixture));
    zassert_not_null(fixture, NULL);

    g_fixture = fixture;
    zassert_equal(g_fixture, fixture, "Invalid fixture setup");

    return fixture;
}

static void test_fixture_before(void *f)
{
    zassert_not_null(g_fixture);

    struct lora_integration_test_fixture *fixture = (struct lora_integration_test_fixture *)f;
    zassert_equal(g_fixture, fixture, "Invalid fixture setup");

    // reset any previously set callback
    fixture->stop_signal = ATOMIC_INIT(0);
    fixture->ctx.stop_signal = &fixture->stop_signal;
    fixture->number_of_received_messages = 0;

    lora_service_setup(&fixture->ctx);

    // assert that lora service configured a callback on setup
    zassert_true(fake_sx128x_is_rx_callback_set());
}

static void test_fixture_after(void *f)
{
    zassert_not_null(g_fixture);

    struct lora_integration_test_fixture *fixture = (struct lora_integration_test_fixture *)f;
    zassert_equal(g_fixture, fixture, "Invalid fixture setup");

    // make thread stop
    atomic_set(&fixture->stop_signal, 1);

    // give some time for thread to stop
    k_sleep(K_MSEC(100));
}

static void test_fixture_teardown(void *f)
{
    g_fixture = NULL;
    free(f);
}

#endif
