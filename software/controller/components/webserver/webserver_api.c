#include "webserver_api.h"
#include "esp_check.h"

#define TAG "WEBSERVER_API"

#define WEBSERVER_APIP_ENDPOINT_LENGTH_MAX (64)
#define WEBSERVER_APIP_ENDPOINT_PREFIX     "/api"

typedef struct {
    webserver_api_handler handler;
    bool allow_cors;
    void *user_ctx;
    char *cors_allowed_methods;
} webserver_api_ctx_wrapper_t;

static esp_err_t webserver_api_handler_wrapper(httpd_req_t *req);
static esp_err_t webserver_api_handler_options_wrapper(httpd_req_t *req);
static webserver_api_handler *webserver_api_handler_get(webserver_api_method_handlers_t *handlers,
                                                        httpd_method_t method);
static esp_err_t httpd_req_recv_blocking(httpd_req_t *r, char *buf, size_t buf_len, TickType_t timeout);

//---------------------------------------------------------------------------------------------------------------------

esp_err_t webserver_api_endpoint_add(webserver_ctx_t *webserver_ctx, const char *uri,
                                     webserver_api_method_handlers_t *handlers, bool allow_cors, void *user_ctx)
{
    /* Validate webserver handle. */
    ESP_RETURN_ON_FALSE(webserver_ctx != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid webserver handle.");

    char cors_allowed_mehods[128] = {0};

    /* Validate uri endpoint length. */
    char uri_endpoint[WEBSERVER_APIP_ENDPOINT_LENGTH_MAX] = WEBSERVER_APIP_ENDPOINT_PREFIX;
    uint8_t uri_offset                                    = strlen(uri_endpoint);
    ESP_RETURN_ON_FALSE(strlen(uri) + uri_offset < WEBSERVER_APIP_ENDPOINT_LENGTH_MAX, ESP_ERR_INVALID_ARG, TAG,
                        "URI too long");

    /* Concatenate api endpoint. */
    strcat(uri_endpoint, uri);

    /* Get the handler. */
    const httpd_method_t methods[] = {HTTP_GET, HTTP_POST, HTTP_PUT, HTTP_DELETE};
    for (uint8_t i = 0; i < sizeof(methods) / sizeof(httpd_method_t); i++) {
        webserver_api_handler *handler = webserver_api_handler_get(handlers, methods[i]);

        if (*handler == NULL) {
            continue;
        }

        /* Create a wrapper context. */
        webserver_api_ctx_wrapper_t *ctx = malloc(sizeof(webserver_api_ctx_wrapper_t));
        ESP_RETURN_ON_FALSE(ctx != NULL, ESP_ERR_NO_MEM, TAG, "Failed to allocate memory %s handler",
                            http_method_str(methods[i]));

        /* Populate wrapper context. */
        ctx->handler    = *handler;
        ctx->allow_cors = allow_cors;
        ctx->user_ctx   = user_ctx;

        /* Configure uri handler structure. */
        httpd_uri_t uri_handler = {
            .uri      = uri_endpoint,
            .method   = methods[i],
            .handler  = webserver_api_handler_wrapper,
            .user_ctx = ctx,
        };

        /* Register uri handler. */
        ESP_LOGI(TAG, "Registering %s handler for %s", http_method_str(uri_handler.method), uri_endpoint);
        ESP_RETURN_ON_ERROR(httpd_register_uri_handler(webserver_ctx->server, &uri_handler), TAG,
                            "Failed to register %s handler", http_method_str(uri_handler.method));

        /* Prepare Access-Control-Allow-Methods value. */
        if (allow_cors) {
            snprintf(cors_allowed_mehods + strlen(cors_allowed_mehods),
                     sizeof(cors_allowed_mehods) - strlen(cors_allowed_mehods), "%s, ",
                     http_method_str(uri_handler.method));
        }
    }

    /* Register the options handler. */
    /* strlen > 0 indicates that at least one handler was registered. */
    if (!allow_cors && handlers->options_handler != NULL && strlen(cors_allowed_mehods) == 0) {
        return ESP_OK;
    }

    /* Remove trailing ", " from cors_allowed_mehods. */
    cors_allowed_mehods[strlen(cors_allowed_mehods) - 2] = '\0';

    /* Create a wrapper context. */
    webserver_api_ctx_wrapper_t *ctx = malloc(sizeof(webserver_api_ctx_wrapper_t));
    ESP_RETURN_ON_FALSE(ctx != NULL, ESP_ERR_NO_MEM, TAG, "Failed to allocate memory OPTIONS handler");

    /* Populate wrapper context. */
    asprintf(&ctx->cors_allowed_methods, "%s", cors_allowed_mehods);
    ESP_RETURN_ON_FALSE(ctx->cors_allowed_methods != NULL, ESP_ERR_NO_MEM, TAG,
                        "Failed to allocate memory OPTIONS handler");

    ctx->handler    = handlers->options_handler;
    ctx->allow_cors = allow_cors;
    ctx->user_ctx   = user_ctx;

    /* Configure uri handler structure. */
    httpd_uri_t uri_handler = {
        .uri      = uri_endpoint,
        .method   = HTTP_OPTIONS,
        .handler  = webserver_api_handler_options_wrapper,
        .user_ctx = ctx,
    };

    /* Register uri handler. */
    ESP_LOGI(TAG, "Registering OPTIONS handler for %s", uri_endpoint);
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(webserver_ctx->server, &uri_handler), TAG,
                        "Failed to register OPTIONS handler");

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t webserver_api_util_file_upload_to_chunk_cb(httpd_req_t *req, webserver_api_util_chunk_handler chunk_handler,
                                                     void *user_ctx, size_t chunk_size)
{
    esp_err_t ret    = ESP_OK; /* Set by ESP_GOTO_ON_ERROR macro. */
    size_t offset    = 0;
    size_t remaining = req->content_len;

    char *buf = malloc(chunk_size);
    ESP_GOTO_ON_FALSE(buf != NULL, ESP_ERR_NO_MEM, exit_no_free, TAG, "Failed to allocate memory for POST data");

    while (remaining > 0) {
        size_t len = remaining < chunk_size ? remaining : chunk_size;

        /* Receive a chunk of data. */
        ESP_GOTO_ON_ERROR(httpd_req_recv_blocking(req, buf, len, pdMS_TO_TICKS(5000)), exit, TAG,
                          "Failed to receive POST data");

        /* Call the chunk handler callback. */
        ESP_GOTO_ON_ERROR(chunk_handler(user_ctx, buf, len, offset, req->content_len), exit, TAG,
                          "Failed to handle chunk");

        /* Update data counters. */
        offset += len;
        remaining -= len;
    }

    /* Send a response. */
exit:
    if (ret == ESP_OK) {
        httpd_resp_sendstr(req, "OK");
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, NULL);
    }

    /* Free the chunk buffer. */
    free(buf);

exit_no_free:
    return ret;
}

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Wrapper function to handle API requests.
 *
 * This wrapper function is used to add CORS headers to the response.
 *
 * \param[in] req The HTTP request.
 *
 * \return esp_err_t
 */
static esp_err_t webserver_api_handler_wrapper(httpd_req_t *req)
{
    /* Get the request handler. */
    webserver_api_ctx_wrapper_t *ctx = (webserver_api_ctx_wrapper_t *)req->user_ctx;

    /* Set CORS headers. */
    if (ctx->allow_cors) {
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    }

    /* Update the context. */
    req->user_ctx = ctx->user_ctx; /* Does this leak memory? */

    /* Call the original handler. */
    return ctx->handler(req);
}

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Wrapper function to handle OPTIONS requests.
 *
 * When an api endpoint is added with allow_cors set to true, the OPTIONS handler is automatically added. If a custom
 * OPTIONS handler is provided, it will be called from within this function.
 *
 * \param[in] req The HTTP request.
 *
 * \return esp_err_t
 */
static esp_err_t webserver_api_handler_options_wrapper(httpd_req_t *req)
{
    /* Get the request handler. */
    webserver_api_ctx_wrapper_t *ctx = (webserver_api_ctx_wrapper_t *)req->user_ctx;

    /* Set CORS headers. */
    if (ctx->allow_cors) {
        httpd_resp_set_status(req, "204 No Content");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", ctx->cors_allowed_methods);
    }

    /* Return if no handler was provided. */
    if (ctx->handler == NULL) {
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }

    /* Update the context. */
    req->user_ctx = ctx->user_ctx; /* Does this leak memory? */

    /* Call the original handler. */
    return ctx->handler(req);
}

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Get the handler for a given HTTP method.
 *
 * \param[in] handlers The method handlers.
 * \param[in] method The HTTP method.
 *
 * \return webserver_api_handler
 */
static webserver_api_handler *webserver_api_handler_get(webserver_api_method_handlers_t *handlers,
                                                        httpd_method_t method)
{
    switch (method) {
        case HTTP_GET:
            return &(handlers->get_handler);
        case HTTP_POST:
            return &(handlers->post_handler);
        case HTTP_PUT:
            return &(handlers->put_handler);
        case HTTP_DELETE:
            return &(handlers->delete_handler);
        case HTTP_OPTIONS:
            return &(handlers->options_handler);
        default:
            return NULL;
    }
}

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Receive data from a request blocking until all data is received.
 *
 * \param[in] r The HTTP request.
 * \param[in] buf The buffer to store the data.
 * \param[in] buf_len The length of the buffer.
 * \param[in] timeout The timeout for the receive operation. The function might not return the moment the timeout is
 *      exceeded.
 *
 * \return esp_err_t
 */
static esp_err_t httpd_req_recv_blocking(httpd_req_t *r, char *buf, size_t buf_len, TickType_t timeout)
{
    ESP_RETURN_ON_FALSE(r != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid request");

    size_t received       = 0;
    TickType_t start_tick = xTaskGetTickCount();

    while (received < buf_len) {
        int recv_len = httpd_req_recv(r, buf + received, buf_len - received);
        if (recv_len < 0) {
            if (recv_len == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Check if we need to timeout. */
                ESP_RETURN_ON_FALSE((xTaskGetTickCount() - start_tick) < timeout, ESP_ERR_TIMEOUT, TAG,
                                    "Receive Timeout");
            } else {
                ESP_LOGE(TAG, "Failed to receive data");
                return ESP_FAIL;
            }
        }
        received += recv_len;
    }

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------