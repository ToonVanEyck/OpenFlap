#include "webserver.h"
#include "esp_check.h"
#include "esp_log.h"

#define TAG "WEBSERVER"

//---------------------------------------------------------------------------------------------------------------------

/* Index http handler. */
static esp_err_t index_page_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_start, index_end - index_start);
    return ESP_OK;
}

static const httpd_uri_t index_page = {
    .uri = "/", .method = HTTP_GET, .handler = index_page_get_handler, .user_ctx = NULL};

//---------------------------------------------------------------------------------------------------------------------

/* Style css handler. */
static esp_err_t style_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)style_start, style_end - style_start);
    return ESP_OK;
}

static const httpd_uri_t style_uri = {
    .uri = "/style.css", .method = HTTP_GET, .handler = style_get_handler, .user_ctx = NULL};

//---------------------------------------------------------------------------------------------------------------------

/* Favicon svg handler. */
static esp_err_t favicon_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "image/svg+xml");
    httpd_resp_send(req, (const char *)favicon_start, favicon_end - favicon_start);
    return ESP_OK;
}

static const httpd_uri_t favicon_uri = {
    .uri = "/favicon.svg", .method = HTTP_GET, .handler = favicon_get_handler, .user_ctx = NULL};

//---------------------------------------------------------------------------------------------------------------------

/* Script js handler. */
static esp_err_t script_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/javascript");
    httpd_resp_set_hdr(req, "Content-Type", "text/javascript; charset=utf-8");
    httpd_resp_send(req, (const char *)script_start, script_end - script_start);
    return ESP_OK;
}

static const httpd_uri_t script_uri = {
    .uri = "/script.js", .method = HTTP_GET, .handler = script_get_handler, .user_ctx = NULL};

//---------------------------------------------------------------------------------------------------------------------

esp_err_t webserver_init(webserver_ctx_t *webserver_ctx)
{
    /* Validate webserver handle. */
    ESP_RETURN_ON_FALSE(webserver_ctx != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid webserver handle.");

    /* Configure HTTP server. */
    httpd_config_t config   = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 20;
    config.server_port      = 80;

    /* Start server. */
    ESP_LOGI(TAG, "Starting web server on port: %d", config.server_port);
    ESP_RETURN_ON_ERROR(httpd_start(&webserver_ctx->server, &config), TAG, "Failed to start web server.");

    /* Register website URL handlers. */
    ESP_LOGI(TAG, "Registering URI handlers");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(webserver_ctx->server, &index_page), TAG,
                        "Failed to register index page handler.");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(webserver_ctx->server, &style_uri), TAG,
                        "Failed to register style handler.");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(webserver_ctx->server, &favicon_uri), TAG,
                        "Failed to register favicon handler.");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(webserver_ctx->server, &script_uri), TAG,
                        "Failed to register script handler.");

    return ESP_OK;
}