/**
 * \file main.c
 * \brief Main file for the controller_v2 project
 * \author Toon Van Eyck
 */

#include "chain_comm_esp.h"
#include "display.h"
#include "driver/gpio.h"
#include "esp_app_desc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "module.h"
#include "module_http_api.h"
#include "networking.h"
#include "oled_disp.h"
#include "utils.h"
#include "webserver.h"

#include <regex.h>
#include <stdio.h>
#include <string.h>

#define TAG "MAIN"

void app_main(void)

{
    const esp_app_desc_t app_desc = *esp_app_get_description();
    ESP_LOGI(TAG, "Starting OpenFlap controller: %s", app_desc.version);

    /* Initialize the oled display. */
    oled_disp_ctx_t oled_disp_ctx;
    oled_disp_init(&oled_disp_ctx);
    uint8_t major, minor, patch;
    util_extract_version(app_desc.version, &major, &minor, &patch);
    oled_disp_home(&oled_disp_ctx, "OPENFLAP", major, minor, patch);

    /* Connect to a network. */
    networking_config_t network_config = NETWORKING_DEFAULT_CONFIG;
    ESP_ERROR_CHECK(networking_setup(&network_config));
    networking_wait_for_connection(10000);
    ESP_LOGI(TAG, "Connected to network!");

    /* Initialize the OpenFlap display. */
    display_t display;
    ESP_ERROR_CHECK(display_init(&display));
    ESP_LOGI(TAG, "Display initialized!");

    /* Initialize the chain communication. */
    chain_comm_ctx_t chain_comm_ctx;
    ESP_ERROR_CHECK(chain_comm_init(&chain_comm_ctx, &display));

    /* Start the web server. */
    webserver_ctx_t webserver_ctx;
    ESP_ERROR_CHECK(webserver_init(&webserver_ctx));
    ESP_LOGI(TAG, "Webserver started!");

    /* Initialize http module api. */
    module_http_api_init(&webserver_ctx, &display);
    ESP_LOGI(TAG, "Module api endpoint started!");

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        // ESP_LOGI(TAG, "Hello world!");
    }
}