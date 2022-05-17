#ifndef FLAP_API_v1_H
#define FLAP_API_v1_H

#include <string.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_http_server.h"
#include "driver/gpio.h"
#include "cJSON.h"

#include "flap_command.h"
#include "flap_nvs.h"
#include "flap_uart.h"

#define FLAP_ENABLE 2

#define LARGE_REQUEST_GUARD(_req)                               \
    if(_req->content_len > CMD_COMM_BUF_LEN){                   \
        httpd_resp_set_status(_req, "413 Payload Too Large");   \
        httpd_resp_send(_req, NULL, 0);                         \
        return ESP_OK;                                          \
    }

esp_err_t modules_get_dimensions(int* width, int* height);
void controller_respons_enqueue(controller_queue_data_t* cmd_comm);
void get_dimensions(cJSON *json, int* width, int* height);
void add_api_endpoints(httpd_handle_t *server);

#endif