#include "packets.h"
#include "zephyr/logging/log.h"
#include "zephyr/toolchain.h"
#include "zephyr/ztest_assert.h"
#include <zephyr/ztest.h>
#include "services/lora.h"
#include "zephyr/ztest_test.h"

#include <invictus2/drivers/sx128x_context.h>
#include "lora_fixture.h"
#include <stdint.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lora_integration_tests, CONFIG_LORA_SX128X_LOG_LEVEL);

ZTEST_SUITE(fake_lora_device_test, NULL, test_fixture_setup, test_fixture_before, NULL,
            test_fixture_teardown);

extern const struct device *fake_sx128x_get_device();
extern bool fake_sx128x_is_rx_callback_set();
extern void fake_sx128x_rf_reception(uint8_t *, size_t);

static void test_on_recv_data(uint8_t *payload, uint16_t size)
{
    g_fixture->ctx.rx_size = size;
    if (payload != NULL)
    {
        g_fixture->received_packet = (struct generic_packet_s *)payload;
    }

    g_fixture->lora_rcv_callback_called = true;
}

ZTEST_F(fake_lora_device_test, test_fixture_callback_call_detected_SUCCESS)
{
    test_on_recv_data(NULL, 0);
    zassert_true(fixture->lora_rcv_callback_called, "Callback not called");
}

ZTEST_F(fake_lora_device_test, test_fake_device_data_callback_not_called_SUCCESS)
{
    zassert_not_null(fake_sx128x_get_device(), "device was NULL");
    zassert_false(fake_sx128x_is_rx_callback_set(), "Rx callback is set");

    sx128x_register_recv_callback(&test_on_recv_data);
    zassert_true(fake_sx128x_is_rx_callback_set(), "No Rx callback set");

    zassert_false(fixture->lora_rcv_callback_called, "Callbacked was called");
}

ZTEST_F(fake_lora_device_test, test_fake_device_data_callback_called_SUCCESS)
{
    zassert_not_null(fake_sx128x_get_device(), "device was NULL");
    zassert_false(fake_sx128x_is_rx_callback_set(), "Rx callback is set");

    sx128x_register_recv_callback(&test_on_recv_data);
    zassert_true(fake_sx128x_is_rx_callback_set(), "No Rx callback set");

    fake_sx128x_rf_reception(NULL, 0);
    zassert_true(fixture->lora_rcv_callback_called, "Callback was NOT called");
}

ZTEST_F(fake_lora_device_test, test_fake_device_data_callback_payload_valid_SUCCESS)
{
    zassert_not_null(fake_sx128x_get_device(), "device was NULL");
    zassert_false(fake_sx128x_is_rx_callback_set(), "Rx callback is set");

    sx128x_register_recv_callback(&test_on_recv_data);
    zassert_true(fake_sx128x_is_rx_callback_set(), "No Rx callback set");

    struct generic_packet_s cmd;
    cmd.header.packet_version = 0x12;
    cmd.header.sender_id = 0x02;
    cmd.header.target_id = 0x01;
    cmd.header.command_id = 0xFE;

    fake_sx128x_rf_reception((uint8_t *)&cmd, sizeof(cmd));

    // assert callback was called
    zassert_true(fixture->lora_rcv_callback_called, "Callback was NOT called");

    // assert correct receive size
    zassert_equal(fixture->ctx.rx_size, sizeof(struct generic_packet_s));

    // assert we're receiving valid packet
    zassert_not_null(fixture->received_packet);
    zassert_true(cmd.header.packet_version == fixture->received_packet->header.packet_version);
    zassert_true(cmd.header.sender_id == fixture->received_packet->header.sender_id);
    zassert_true(cmd.header.target_id == fixture->received_packet->header.target_id);
    zassert_true(cmd.header.command_id == fixture->received_packet->header.command_id);
}
