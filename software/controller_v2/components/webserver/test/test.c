#include "esp_log.h"
#include "memory_checks.h"
#include "networking.h"
#include "unity.h"
#include "webserver.h"
#include <stdio.h>

#define TAG "WEBSERVER_TEST"

TEST_CASE("Test webserver", "[webserver][qemu]")
{
    /* Configure Ethernet for testing on qemu. */
    networking_config_t config = NETWORKING_DEFAULT_CONFIG;
    networking_setup(&config);
    TEST_ASSERT_EQUAL(networking_wait_for_connection(10000), ESP_OK);

    TEST_ASSERT_EQUAL(webserver_init(), ESP_OK);

    ESP_LOGI(TAG, "Webserver started");

    char dummy[16] = {0};
    unity_gets(dummy, sizeof(dummy));

    ESP_LOGI(TAG, "Webserver stopped");

    test_utils_record_free_mem();
    TEST_ASSERT(true);
}
