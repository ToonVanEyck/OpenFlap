#ifndef API_H
#define API_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cJSON.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Model.h"
#include "UartApi.h"

#define MAX_HTTP_BODY_SIZE 2048

#define LARGE_REQUEST_GUARD(_req)                                                                                      \
    if (_req->content_len > MAX_HTTP_BODY_SIZE) {                                                                      \
        httpd_resp_set_status(_req, "413 Payload Too Large");                                                          \
        httpd_resp_send(_req, NULL, 0);                                                                                \
        return ESP_OK;                                                                                                 \
    }

void http_moduleEndpointInit();

#endif