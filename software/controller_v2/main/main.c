/**
 * \file main.c
 * \brief Main file for the controller_v2 project
 * \author Toon Van Eyck
 */

#include "chain_comm_esp.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
// #include "module.h"
// #include "module_http_api.h"
#include "display.h"
#include "networking.h"
#include "oled_disp.h"
#include "webserver.h"

#define TAG "MAIN"

void app_main(void)

{
    /* Fucking enable pin for UART and Relay on old controller. I removed the flipping relay */
    // gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    // gpio_set_level(GPIO_NUM_2, 1);

    ESP_LOGI(TAG, "Starting OpenFlap controller!");

    oled_disp_init();

    /* Connect to a newtork. */
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

    // /* Initialize http module api. */
    // module_http_api_init(&webserver_ctx, &display);
    // ESP_LOGI(TAG, "Module api endpoint started!");

    // while (1) {
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    //     ESP_LOGI(TAG, "Hello world!");
    // }
}
