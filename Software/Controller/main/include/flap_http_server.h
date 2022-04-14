#ifndef FLAP_HTTP_H
#define FLAP_HTTP_H

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_http_server.h>
#include "cJSON.h"

#include <regex.h>

#include "flap_controller.h"

extern const uint8_t index_start[]          asm("_binary_index_html_start");
extern const uint8_t index_end[]            asm("_binary_index_html_end");
extern const uint8_t style_start[]          asm("_binary_style_css_start");
extern const uint8_t style_end[]            asm("_binary_style_css_end");
extern const uint8_t favicon_start[]        asm("_binary_favicon_svg_start");
extern const uint8_t favicon_end[]          asm("_binary_favicon_svg_end");

httpd_handle_t flap_start_webserver(void);
esp_err_t trigger_async_send(char *json_data);
void http_server_disconnect_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data);
void http_server_connect_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data);
void flap_init_webserver();

#endif