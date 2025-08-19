/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/logging/log.h"
#include "zephyr/ztest_assert.h"
#include <zephyr/ztest.h>

#include <invictus2/drivers/sx128x_context.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lora_integration_tests, CONFIG_LORA_SX128X_LOG_LEVEL);

ZTEST_SUITE(lora_integration_tests, NULL, NULL, NULL, NULL, NULL);

extern const struct device *fake_sx128x_get_device();
extern bool fake_sx128x_is_rx_callback_set();

static void test_on_recv_data(uint8_t *payload, uint16_t size)
{
    // TODO implement
}

ZTEST(lora_integration_tests, test_fake_device_data)
{
    zassert_not_null(fake_sx128x_get_device(), "device was NULL");
    zassert_false(fake_sx128x_is_rx_callback_set(), "Rx callback is set");

    sx128x_register_recv_callback(&test_on_recv_data);
    zassert_true(fake_sx128x_is_rx_callback_set(), "No Rx callback set");
}
