#pragma once

#include "esp_http_server.h"

/**
 * \brief Context of the webserver
 */
typedef struct {
    httpd_handle_t server; /**< Handle to the HTTP server */
} webserver_ctx_t;
