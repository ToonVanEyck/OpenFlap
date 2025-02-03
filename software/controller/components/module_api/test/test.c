#include "cJSON.h"
#include "display.h"
#include "esp_err.h"
#include "esp_system.h"
#include "memory_checks.h"
#include "module.h"
#include "module_api.h"
#include "networking.h"
#include "properties.h"
#include "unity.h"
#include "webserver.h"

#define TAG "module_api_TEST"

TEST_CASE("Test module http API post handler", "[module_api][qemu]")
{
    /* Configure Ethernet for testing on qemu. */
    networking_config_t config = NETWORKING_DEFAULT_CONFIG;
    networking_setup(&config);
    TEST_ASSERT_EQUAL(networking_wait_for_connection(10000), ESP_OK);

    webserver_ctx_t webserver_ctx;
    TEST_ASSERT_EQUAL(webserver_init(&webserver_ctx), ESP_OK);

    display_t display;
    TEST_ASSERT_EQUAL(display_init(&display), ESP_OK);
    TEST_ASSERT_EQUAL(module_api_init(&webserver_ctx, &display), ESP_OK);

    ESP_LOGI(TAG, "Webserver started");

    /* Wait for pytest to send a character. */
    char dummy[16] = {0};
    unity_gets(dummy, sizeof(dummy));
    /* Continue */

    ESP_LOGI(TAG, "Webserver stopped");

    test_utils_record_free_mem();
}