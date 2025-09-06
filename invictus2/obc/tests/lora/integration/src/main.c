#include "zephyr/logging/log.h"
#include "zephyr/ztest_assert.h"
#include <zephyr/ztest.h>
#include "zephyr/ztest_test.h"

#include <invictus2/drivers/sx128x_context.h>
#include "lora_fixture.h"
#include <stdint.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lora_integration_tests, CONFIG_LORA_SX128X_LOG_LEVEL);

ZTEST_SUITE(lora_integration_test, NULL, test_fixture_setup, test_fixture_before,
            test_fixture_after, test_fixture_teardown);

ZTEST_F(lora_integration_test, test_basic)
{
    zassert_true(1 == 1);
}
