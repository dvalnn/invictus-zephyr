#include "packets.h"
#include "services/lora.h"
#include "syscalls/kernel.h"
#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include "zephyr/sys/atomic.h"
#include "zephyr/ztest_assert.h"
#include <zephyr/ztest.h>
#include "zephyr/ztest_test.h"

#include <invictus2/drivers/sx128x_context.h>
#include "lora_fixture.h"
#include <stdint.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lora_integration_tests, CONFIG_LORA_SX128X_LOG_LEVEL);

static bool radio_command_cmp(const struct generic_packet_s *lhs,
                              const struct generic_packet_s *rhs)
{
    return lhs->header.packet_version == rhs->header.packet_version &&
           lhs->header.sender_id == rhs->header.sender_id &&
           lhs->header.target_id == rhs->header.target_id &&
           lhs->header.command_id == rhs->header.command_id;
}

ZTEST_SUITE(lora_integration_test, NULL, test_fixture_setup, test_fixture_before,
            test_fixture_after, test_fixture_teardown);

ZTEST_F(lora_integration_test, test_lora_service_basic_data_reception_SUCCESS)
{
    // assert no data available before calback
    zassert_not_equal(k_sem_take(&fixture->ctx.data_available, K_NO_WAIT), 0);

    struct generic_packet_s cmd;
    cmd.header.packet_version = 0x12;
    cmd.header.sender_id = 0x02;
    cmd.header.target_id = 0x01;
    cmd.header.command_id = 0xFE;

    fake_sx128x_rf_reception((uint8_t *)&cmd, sizeof(cmd));

    // assert size of written data
    zassert_equal(fixture->ctx.rx_size, sizeof(struct generic_packet_s));
    // assert data available triggered after callback
    zassert_equal(k_sem_take(&fixture->ctx.data_available, K_NO_WAIT), 0);
}

ZTEST_F(lora_integration_test, test_lora_service_multiple_data_reception_SUCCESS)
{
    // assert no data available before calback
    zassert_not_equal(k_sem_take(&fixture->ctx.data_available, K_NO_WAIT), 0);

    struct generic_packet_s cmd;
    cmd.header.packet_version = 0x12;
    cmd.header.sender_id = 0x02;
    cmd.header.target_id = 0x01;
    cmd.header.command_id = 0xFE;

    fake_sx128x_rf_reception((uint8_t *)&cmd, sizeof(cmd));
    fake_sx128x_rf_reception((uint8_t *)&cmd, sizeof(cmd));

    // assert size of written data
    zassert_equal(fixture->ctx.rx_size, 2 * sizeof(struct generic_packet_s));
    // assert no data available triggered after callback
    zassert_equal(k_sem_take(&fixture->ctx.data_available, K_NO_WAIT), 0);
}

ZTEST_F(lora_integration_test, test_lora_service_handle_packet_SUCCESS)
{
    lora_service_start();

    struct generic_packet_s cmd;
    cmd.header.packet_version = 0x12;
    cmd.header.sender_id = 0x02;
    cmd.header.target_id = 0x01;
    cmd.header.command_id = 0xFE;

    fake_sx128x_rf_reception((uint8_t *)&cmd, sizeof(cmd));
    k_sleep(K_MSEC(100));

    zassert_equal(g_fixture->number_of_received_messages, 1);
    zassert_true(radio_command_cmp(&cmd, &g_fixture->received_messages[0]));

    // stop lora service
    atomic_set(&g_fixture->stop_signal, 1);
    // wait some time for thread to finish
    k_sleep(K_MSEC(100));
}

ZTEST_F(lora_integration_test, test_lora_service_handle_multiple_packet_SUCCESS)
{
    lora_service_start();

    struct generic_packet_s cmd;
    cmd.header.packet_version = 0x12;
    cmd.header.sender_id = 0x02;
    cmd.header.target_id = 0x01;
    cmd.header.command_id = 0xFE;

    struct generic_packet_s cmd2 = cmd;
    cmd2.header.command_id = 0xAA;

    fake_sx128x_rf_reception((uint8_t *)&cmd, sizeof(cmd));
    k_sleep(K_MSEC(10));
    fake_sx128x_rf_reception((uint8_t *)&cmd2, sizeof(cmd2));
    k_sleep(K_MSEC(100));

    zassert_equal(g_fixture->number_of_received_messages, 2);
    zassert_true(radio_command_cmp(&cmd, &g_fixture->received_messages[0]));
    zassert_true(radio_command_cmp(&cmd2, &g_fixture->received_messages[1]));

    // stop lora service
    atomic_set(&g_fixture->stop_signal, 1);
    // wait some time for thread to finish
    k_sleep(K_MSEC(100));
}
