#include "esp_err.h"
#include "esp_system.h"
#include "memory_checks.h"
#include "networking.h"
#include "unity.h"

TEST_CASE("test default networking", "[networking][qemu][target]")
{
    networking_config_t config = NETWORKING_DEFAULT_CONFIG;
    TEST_ASSERT_EQUAL(ESP_OK, networking_setup(&config));
    TEST_ASSERT_EQUAL(ESP_OK, networking_wait_for_connection(5000));
    test_utils_record_free_mem();
}
