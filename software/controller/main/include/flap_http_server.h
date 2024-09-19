#ifndef FLAP_HTTP_H
#define FLAP_HTTP_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_http_server.h"
#include "cJSON.h"

#include <regex.h>

#include "HttpApi.h"
#include "flap_firmware.h"
#include "chain_comm_abi.h"
#include "flap_nvs.h"

extern const uint8_t index_start[]          asm("_binary_index_html_start");
extern const uint8_t index_end[]            asm("_binary_index_html_end");
extern const uint8_t style_start[]          asm("_binary_style_css_start");
extern const uint8_t style_end[]            asm("_binary_style_css_end");
extern const uint8_t favicon_start[]        asm("_binary_favicon_svg_start");
extern const uint8_t favicon_end[]          asm("_binary_favicon_svg_end");
extern const uint8_t script_start[]        asm("_binary_script_js_start");
extern const uint8_t script_end[]          asm("_binary_script_js_end");

typedef bool (*http_modulePropertyCallback_t)(cJSON**, module_t*);

typedef struct{
    http_modulePropertyCallback_t toJson;
    http_modulePropertyCallback_t fromJson;
}http_modulePropertyHandler_t;
void http_addModulePropertyHandler(moduleProperty_t property, http_modulePropertyCallback_t toJson, http_modulePropertyCallback_t fromJson);

httpd_handle_t flap_start_webserver(void);
esp_err_t trigger_async_send(char *json_data);
void http_server_disconnect_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data);
void http_server_connect_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data);
void flap_init_webserver();

TaskHandle_t httpTask();



#endif