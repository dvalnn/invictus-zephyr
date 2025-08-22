#ifndef OBC_TESTS_LORA_H
#define OBC_TESTS_LORA_H

#include <stdlib.h>
#include <stdbool.h>
#include "zephyr/ztest_test.h"
#include "services/lora.h"

struct lora_integration_fixture
{
    bool lora_rcv_callback_called;
    atomic_t stop_signal;
    lora_context_t ctx;
};

// Hack to access fixture on callbacks
static struct lora_integration_fixture *g_fixture = NULL;

static void *lora_fixture_setup(void)
{
    /* Allocate the fixture with 256 byte buffer */
    struct lora_integration_fixture *fixture = malloc(sizeof(struct lora_integration_fixture));

    zassume_not_null(fixture, NULL);
    g_fixture = fixture;
    return fixture;
}

static void lora_fixture_before(void *f)
{
    struct lora_integration_fixture *fixture = (struct lora_integration_fixture *)f;
    fixture->lora_rcv_callback_called = false;
    fixture->stop_signal = ATOMIC_INIT(0);
    fixture->ctx.stop_signal = &fixture->stop_signal;

    zassume_not_null(g_fixture, NULL);
}

static void lora_fixture_teardown(void *f)
{
    g_fixture = NULL;
    free(f);
}

#endif
