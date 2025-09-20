#ifndef OBC_TESTS_LORA_H
#define OBC_TESTS_LORA_H

#include <stdlib.h>
#include <stdbool.h>
#include "packets.h"
#include "zephyr/ztest_assert.h"
#include <invictus2/drivers/sx128x_context.h>
#include "services/lora.h"

struct fake_lora_device_test_fixture
{
    bool lora_rcv_callback_called;
    atomic_t stop_signal;
    lora_context_t ctx;
    struct generic_packet_s *received_packet;
};

// Hack to access fixture on callbacks
static struct fake_lora_device_test_fixture *g_fixture = NULL;

static void *test_fixture_setup(void)
{
    zassert_is_null(g_fixture);
    struct fake_lora_device_test_fixture *fixture =
        malloc(sizeof(struct fake_lora_device_test_fixture));
    zassert_not_null(fixture, NULL);

    g_fixture = fixture;
    zassert_equal(g_fixture, fixture, "Invalid fixture setup");

    return fixture;
}

static void test_fixture_before(void *f)
{
    zassert_not_null(g_fixture);

    struct fake_lora_device_test_fixture *fixture = (struct fake_lora_device_test_fixture *)f;
    zassert_equal(g_fixture, fixture, "Invalid fixture setup");

    // reset any previously set callback
    sx128x_register_recv_callback(NULL);
    fixture->lora_rcv_callback_called = false;
    fixture->stop_signal = ATOMIC_INIT(0);
    fixture->ctx.stop_signal = &fixture->stop_signal;
    fixture->received_packet = NULL;
}

static void test_fixture_teardown(void *f)
{
    g_fixture = NULL;
    free(f);
}

#endif
