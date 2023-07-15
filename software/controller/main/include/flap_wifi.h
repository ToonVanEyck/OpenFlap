#ifndef FLAP_WIFI_H
#define FLAP_WIFI_H

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "flap_mdns.h"

void flap_wifi_init_apsta(char* sta_ssid, char*  sta_pwd, char* ap_ssid, char*  ap_pwd);

#endif