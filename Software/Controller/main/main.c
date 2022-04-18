#include <stdio.h>
#include "flap_nvs.h"
#include "flap_wifi.h"
#include "flap_http_server.h"
#include "flap_socket_server.h"
#include "flap_mdns.h"
#include "flap_controller.h"
#include "flap_firmware.h"
#include "flap_uart.h"

#define FLAP_ENABLE 2 

static const char *TAG = "[MAIN]";

void app_main(void)
{
    gpio_set_direction(FLAP_ENABLE, GPIO_MODE_OUTPUT);
    gpio_set_level(FLAP_ENABLE, 1);

    flap_verify_controller_firmware();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    flap_init_webserver();
    nvs_init();
    char *wifi_pwd,*wifi_ssid;
    flap_nvs_get_string("wifi_ssid",&wifi_ssid);
    flap_nvs_get_string("wifi_password",&wifi_pwd);
    flap_mdns_init("openflap");
    flap_wifi_init_apsta(wifi_ssid,wifi_pwd,"OpenFlap","OpenFlap");

    flap_init_socket_server();

    flap_controller_init();
    flap_uart_init();

}
