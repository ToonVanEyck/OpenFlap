#include "webserver_api.h"
#include "esp_check.h"

#define TAG "WEBSERVER_API"

#define WEBSERVER_APIP_ENDPOINT_LENGTH_MAX (64)
#define WEBSERVER_APIP_ENDPOINT_PREFIX     "/api"

esp_err_t webserver_api_endpoint_add(webserver_ctx_t *webserver_ctx, const char *uri, httpd_method_t method,
                                     webserver_api_handler handler, void *user_ctx)
{
    /* Validate webserver handle. */
    ESP_RETURN_ON_FALSE(webserver_ctx != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid webserver handle.");

    /* Validate uri endpoint length. */
    char uri_endpoint[WEBSERVER_APIP_ENDPOINT_LENGTH_MAX] = WEBSERVER_APIP_ENDPOINT_PREFIX;
    uint8_t uri_offset                                    = strlen(uri_endpoint);
    ESP_RETURN_ON_FALSE(strlen(uri) + uri_offset < WEBSERVER_APIP_ENDPOINT_LENGTH_MAX, ESP_ERR_INVALID_ARG, TAG,
                        "URI too long");

    /* Concatenate api endpoint. */
    strcat(uri_endpoint, uri);

    /* Configure uri handler structure.  */
    httpd_uri_t uri_handler = {
        .uri      = uri_endpoint,
        .method   = method,
        .handler  = handler,
        .user_ctx = user_ctx,
    };

    /* Register uri handler. */
    return httpd_register_uri_handler(webserver_ctx->server, &uri_handler);
}