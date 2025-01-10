#include "esp_err.h"
#include "memory_checks.h"
#include "oled_disp.h"
#include "unity.h"

#define TAG "OLED_DISP_TEST"

TEST_CASE("Test oled_disp_home", "[oled_disp][target]")
{
    oled_disp_ctx_t ctx;
    TEST_ASSERT_EQUAL(ESP_OK, oled_disp_init(&ctx));
    TEST_ASSERT_EQUAL(ESP_OK, oled_disp_home(&ctx, "OPENFLAP", 1, 2, 3));
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    TEST_ASSERT_EQUAL(ESP_OK, oled_disp_clean(&ctx));
}

TEST_CASE("Test oled_disp_loading", "[oled_disp][target]")
{
    oled_disp_ctx_t ctx;
    TEST_ASSERT_EQUAL(ESP_OK, oled_disp_init(&ctx));
    for (uint8_t i = 0; i < 100; i += 5) {
        TEST_ASSERT_EQUAL(ESP_OK, oled_disp_loading(&ctx, "Loading FW:", i));
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
    TEST_ASSERT_EQUAL(ESP_OK, oled_disp_clean(&ctx));
}

TEST_CASE("Test oled_disp_wifi_mode", "[oled_disp][target]")
{
    oled_disp_ctx_t ctx;
    TEST_ASSERT_EQUAL(ESP_OK, oled_disp_init(&ctx));

    TEST_ASSERT_EQUAL(ESP_OK, oled_disp_wifi_mode(&ctx, false, false));
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    TEST_ASSERT_EQUAL(ESP_OK, oled_disp_wifi_mode(&ctx, true, false));
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    TEST_ASSERT_EQUAL(ESP_OK, oled_disp_wifi_mode(&ctx, false, true));
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    TEST_ASSERT_EQUAL(ESP_OK, oled_disp_wifi_mode(&ctx, true, true));
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    TEST_ASSERT_EQUAL(ESP_OK, oled_disp_clean(&ctx));
}

TEST_CASE("Test oled_disp_wifi_ap", "[oled_disp][target]")
{
    oled_disp_ctx_t ctx;
    TEST_ASSERT_EQUAL(ESP_OK, oled_disp_init(&ctx));

    TEST_ASSERT_EQUAL(ESP_OK, oled_disp_wifi_ap(&ctx, "open_flap_ssid", 3));
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    TEST_ASSERT_EQUAL(ESP_OK, oled_disp_clean(&ctx));
}

TEST_CASE("Test oled_disp_wifi_ap", "[oled_disp][target]")
{
    oled_disp_ctx_t ctx;
    TEST_ASSERT_EQUAL(ESP_OK, oled_disp_init(&ctx));

    TEST_ASSERT_EQUAL(ESP_OK, oled_disp_wifi_sta(&ctx, "open_flap", true, "192.168.0.123"));
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    TEST_ASSERT_EQUAL(ESP_OK, oled_disp_clean(&ctx));
}