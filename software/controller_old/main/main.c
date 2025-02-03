#include "Model.h"
#include "board_io.h"
#include "flap_firmware.h"
#include "flap_http_server.h"
#include "flap_mdns.h"
#include "flap_nvs.h"
#include "flap_socket_server.h"
#include "flap_uart.h"
#include "flap_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include <stdio.h>

static const char *TAG = "[MAIN]";

TimerHandle_t xTimer;

void timerCallback(TimerHandle_t pxTimer)
{
    /* The timer expired, therefore 5 seconds must have passed since a key
    was pressed.  Switch off the LCD back-light. */
    ESP_LOGI(TAG, "Timer done");
    setLed(0, 0xff, 0);
}

void app_main(void)
{
    nvs_init();
    // The enable pin controls the relay to power the modules.
    board_io_init(xTaskGetCurrentTaskHandle());

    xTimer = xTimerCreate("Boot Config", (500 / portTICK_PERIOD_MS), false, 0, timerCallback);
    if (xTimerStart(xTimer, 0) != pdPASS) {
        ESP_LOGE(TAG, "Timer could not be started");
    } else {
        ESP_LOGI(TAG, "Timer started");
        setLed(0xff, 0, 0);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
    while (xTimerIsTimerActive(xTimer)) {
        uint32_t btn = ulTaskNotifyTake(true, 100 / portTICK_PERIOD_MS);
        if (btn) {
            ESP_LOGI(TAG, "Button pressed");
            if (xTimerReset(xTimer, 10) != pdPASS) {
                /* The reset command was not executed successfully.  Take appropriate
                action here. */
            }
        }
        ESP_LOGI(TAG, "Timer active");
        // vTaskDelay();
    };

    // uint32_t statup_config = ulTaskNotifyTake(true, portMAX_DELAY);

    // initialize components
    flap_verify_controller_firmware();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    flap_init_webserver();
    // read Wi-Fi credentials from NVM
    char *STA_pwd, *STA_ssid, *AP_pwd, *AP_ssid;
    flap_nvs_get_string("STA_ssid", &STA_ssid);
    if (!STA_ssid) {
        asprintf(&STA_ssid, "%c", 0);
    }
    flap_nvs_get_string("STA_pwd", &STA_pwd);
    if (!STA_pwd) {
        asprintf(&STA_pwd, "%c", 0);
    }
    flap_nvs_get_string("AP_ssid", &AP_ssid);
    if (!AP_ssid) {
        asprintf(&AP_ssid, "OpenFlap");
    }
    flap_nvs_get_string("AP_pwd", &AP_pwd);
    if (!AP_pwd) {
        asprintf(&AP_pwd, "myOpenFlap");
    }
    // Set local hostname
    flap_mdns_init("openflap");
    // Start Wi-Fi
    flap_wifi_init_apsta(STA_ssid, STA_pwd, AP_ssid, AP_pwd);
    // start socket server for debugging
    flap_init_socket_server();
    // init uart
    flap_uart_init();
    flap_model_init();
    ESP_LOGI(TAG, "OpenFlap Controller started!");

    gpio_set_level(FLAP_ENABLE_PIN, 1);
}
