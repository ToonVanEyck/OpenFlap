/**
 * \file main.c
 * \brief Main file for the controller project
 * \author Toon Van Eyck
 */

#include "controller_ota.h"
#include "driver/gpio.h"
#include "esp_app_desc.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "module.h"
#include "module_api.h"
#include "networking.h"
#include "openflap_display.h"
// #include "openflap_cc_master.h"
#include "utils.h"
#include "webserver.h"

#include <regex.h>
#include <stdio.h>
#include <string.h>

#define TAG "MAIN"

#define STARTUP_ERROR_CHECK

void app_main(void)

{
    esp_err_t ret                 = ESP_OK; /* Set by ESP_GOTO_ON_ERROR macro. */
    const esp_app_desc_t app_desc = *esp_app_get_description();
    ESP_LOGI(TAG, "Starting OpenFlap controller: %s", app_desc.version);

    /* Connect to a network. */
    networking_config_t network_config = NETWORKING_DEFAULT_CONFIG;
    ESP_GOTO_ON_ERROR(networking_setup(&network_config), verify_firmware, TAG, "Failed to setup networking");
    networking_wait_for_connection(10000);
    ESP_LOGI(TAG, "Connected to network!");

    /* Initialize the OpenFlap display. */
    of_display_t display;
    ESP_GOTO_ON_ERROR(of_display_init(&display), verify_firmware, TAG, "Failed to initialize OpenFlap display");
    ESP_LOGI(TAG, "Display initialized!");

    // /* Initialize the chain communication. */
    // of_cc_master_ctx_t chain_comm_ctx;
    // ESP_GOTO_ON_ERROR(of_cc_master_init(&chain_comm_ctx, &display), verify_firmware, TAG,
    //                   "Failed to initialize chain comm");

    // /* Start the web server. */
    // webserver_ctx_t webserver_ctx;
    // ESP_GOTO_ON_ERROR(webserver_init(&webserver_ctx), verify_firmware, TAG, "Failed to start web server");
    // ESP_LOGI(TAG, "Webserver started!");

    // /* Initialize http module api. */
    // ESP_GOTO_ON_ERROR(module_api_init(&webserver_ctx, &display), verify_firmware, TAG,
    //                   "Failed to initialize module api");
    // ESP_LOGI(TAG, "Module api endpoint started!");

    // /* Initialize controller OTA. */
    // controller_ota_ctx_t controller_ota_ctx;
    // ESP_GOTO_ON_ERROR(controller_ota_init(&controller_ota_ctx, &webserver_ctx), verify_firmware, TAG,
    //                   "Failed to initialize controller OTA");
    // ESP_LOGI(TAG, "Controller OTA endpoint started!");

verify_firmware:
    /* Verify the firmware. */
    bool startup_success = (ret == ESP_OK);
    ESP_ERROR_CHECK(controller_ota_verify_firmware(startup_success));

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGI(TAG, "Hello world!");
    }
}