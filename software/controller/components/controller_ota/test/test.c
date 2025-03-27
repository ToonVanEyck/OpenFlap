#include "controller_ota.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "memory_checks.h"
#include "networking.h"
#include "unity.h"
#include "webserver.h"

#define TAG "CONTROLLER_OTA_TEST"

/* Test Command:
curl -T build/controller/openflap-controller.bin http://openflap.local/api/controller/firmware
*/

TEST_CASE("Test controller OTA post handler", "[controller_ota][qemu]")
{
    /* Configure Ethernet for testing on qemu. */
    networking_config_t config = NETWORKING_DEFAULT_CONFIG;
    networking_setup(&config);
    TEST_ASSERT_EQUAL(networking_wait_for_connection(10000), ESP_OK);

    webserver_ctx_t webserver_ctx;
    TEST_ASSERT_EQUAL(webserver_init(&webserver_ctx), ESP_OK);
    TEST_ASSERT_EQUAL(controller_ota_init(&webserver_ctx), ESP_OK);

    ESP_LOGI(TAG, "Webserver started");

    char dummy[16] = {0};
    unity_gets(dummy, sizeof(dummy));

    ESP_LOGI(TAG, "Webserver stopped");

    test_utils_record_free_mem();
}