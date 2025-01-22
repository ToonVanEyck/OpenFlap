#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t module_api_get_handler(httpd_req_t *req);
esp_err_t module_api_post_handler(httpd_req_t *req);