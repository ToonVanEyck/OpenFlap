#include "display.h"
#include "esp_err.h"
#include "esp_system.h"
#include "memory_checks.h"
#include "unity.h"

#define TAG "DISPLAY_TEST"

TEST_CASE("Test display_init display_destroy", "[display][qemu][target]")
{
    display_t display;
    TEST_ASSERT_EQUAL(ESP_OK, display_init(&display));
    TEST_ASSERT_NOT_NULL(display.event_handle);
    TEST_ASSERT_EQUAL(ESP_OK, display_destroy(&display));
    TEST_ASSERT_NULL(display.event_handle);
}

TEST_CASE("Test display_init display_destroy: ok", "[display][qemu][target]")
{
    display_t display;
    TEST_ASSERT_EQUAL(ESP_OK, display_init(&display));
    TEST_ASSERT_NOT_NULL(display.event_handle);

    TEST_ASSERT_EQUAL(ESP_OK, display_resize(&display, 10));

    uint16_t module_count = display_size_get(&display);
    for (uint16_t i = 0; i < module_count; i++) {
        module_t *module         = display_module_get(&display, i);
        module->character_set    = character_set_new(48);
        module->firmware_version = firmware_version_new(18);
        module->firmware_update  = firmware_update_new();
    }

    /* Shrink Display. */
    TEST_ASSERT_EQUAL(ESP_OK, display_resize(&display, 5));

    /* Grow Display. */
    TEST_ASSERT_EQUAL(ESP_OK, display_resize(&display, 10));

    TEST_ASSERT_EQUAL(ESP_OK, display_destroy(&display));
    TEST_ASSERT_NULL(display.event_handle);
}