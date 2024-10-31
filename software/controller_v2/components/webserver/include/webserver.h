#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

extern const uint8_t index_start[] asm("_binary_index_html_start");
extern const uint8_t index_end[] asm("_binary_index_html_end");
extern const uint8_t style_start[] asm("_binary_style_css_start");
extern const uint8_t style_end[] asm("_binary_style_css_end");
extern const uint8_t favicon_start[] asm("_binary_favicon_svg_start");
extern const uint8_t favicon_end[] asm("_binary_favicon_svg_end");
extern const uint8_t script_start[] asm("_binary_script_js_start");
extern const uint8_t script_end[] asm("_binary_script_js_end");

/**
 * \brief Context of the webserver
 */
typedef struct {
    httpd_handle_t server; /**< Handle to the HTTP server */
} webserver_ctx_t;

/**
 * \brief Handle to the webserver
 */
typedef uintptr_t webserver_handle_t;

/**
 * \brief Initialize the webserver
 *
 * Initialize the webserver with the default configuration. Currently only one webserver is supported and it will be
 * started on port 80.
 *
 * \param[out] webserver_handle Handle to the webserver to be initialized
 *
 * \return esp_err_t
 */
esp_err_t webserver_init(webserver_handle_t webserver_handle);