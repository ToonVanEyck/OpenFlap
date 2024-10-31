/**
 * \file main.c
 * \brief Main file for the controller_v2 project
 * \author Toon Van Eyck
 */

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "networking.h"
#include "webserver.h"
#include <stdio.h>

#define TAG "MAIN"

void app_main(void)
{
    ESP_LOGI(TAG, "Starting OpenFlap controller!");
    networking_config_t network_config = NETWORKING_DEFAULT_CONFIG;
    networking_setup(&network_config);
    networking_wait_for_connection(10000);
    ESP_LOGI(TAG, "Connected to network!");
    webserver_init();

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
