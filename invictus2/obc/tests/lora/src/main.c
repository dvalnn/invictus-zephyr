#include "zephyr/logging/log.h"
#include "zephyr/ztest_assert.h"
#include <zephyr/ztest.h>
#include "services/lora.h"
#include "zephyr/ztest_test.h"

#include <invictus2/drivers/sx128x_context.h>
#include "lora_fixture.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lora_integration_tests, CONFIG_LORA_SX128X_LOG_LEVEL);

ZTEST_SUITE(lora_integration_tests, NULL, NULL, NULL, NULL, NULL);
ZTEST_SUITE(lora_integration, NULL, lora_fixture_setup, lora_fixture_before, NULL,
            lora_fixture_teardown);

extern const struct device *fake_sx128x_get_device();
extern bool fake_sx128x_is_rx_callback_set();

static void test_on_recv_data(uint8_t *payload, uint16_t size)
{
    g_fixture->lora_rcv_callback_called = true;
}

ZTEST(lora_integration_tests, test_lora_service_thread_setup_with_null_context_FAILS)
{
    zassert_false(lora_service_setup(NULL), "Setup lora thread with NULL context");
}

ZTEST(lora_integration_tests, test_lora_service_thread_setup_with_valid_context_SUCCESS)
{
    atomic_t stop_source;
    lora_context_t ctx = {
        .stop_signal = &stop_source,
    };

    zassert_true(lora_service_setup(&ctx), "Setup lora thread faled");
}

ZTEST_F(lora_integration, test_fixture_callback_call_detected_SUCCESS)
{
    test_on_recv_data(NULL, 0);
    zassert_true(fixture->lora_rcv_callback_called, "Callback not called");
}

ZTEST_F(lora_integration, test_fake_device_data_callback_not_called_SUCCESS)
{
    zassert_not_null(fake_sx128x_get_device(), "device was NULL");
    zassert_false(fake_sx128x_is_rx_callback_set(), "Rx callback is set");

    sx128x_register_recv_callback(&test_on_recv_data);
    zassert_true(fake_sx128x_is_rx_callback_set(), "No Rx callback set");

    zassert_false(fixture->lora_rcv_callback_called, "Callbacked was called");
}

ZTEST_F(lora_integration, test_lora_service_thread_setup_with_valid_context_SUCCESS)
{
    zassert_true(lora_service_setup(&fixture->ctx), "Setup lora thread faled");
}
