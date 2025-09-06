#ifndef OBC_TESTS_LORA_H
#define OBC_TESTS_LORA_H

#include <stdlib.h>
#include <stdbool.h>
#include "radio_commands.h"
#include "zephyr/sys/atomic.h"
#include "zephyr/ztest_assert.h"
#include <invictus2/drivers/sx128x_context.h>
#include "services/lora.h"

struct lora_integration_test_fixture
{
    atomic_t stop_signal;
    lora_context_t ctx;
};

// Hack to access fixture on callbacks
static struct lora_integration_test_fixture *g_fixture = NULL;

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

    lora_service_setup(&fixture->ctx);
    lora_service_start();
}

static void test_fixture_after(void *f)
{
    zassert_not_null(g_fixture);

    struct lora_integration_test_fixture *fixture = (struct lora_integration_test_fixture *)f;
    zassert_equal(g_fixture, fixture, "Invalid fixture setup");

    // make thread stop
    atomic_set(&fixture->stop_signal, 1);
}

static void test_fixture_teardown(void *f)
{
    g_fixture = NULL;
    free(f);
}

#endif
