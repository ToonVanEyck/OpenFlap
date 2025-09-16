#include "webserver.h"
#include "esp_check.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/message_buffer.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"

#define TAG "WEBSERVER"

#define WEBSOCKET_LOG_TASK_STACK_SIZE 4096
#define WEBSOCKET_LOG_TASK_PRIO       5
#define WEBSOCKET_LOG_MSG_BUF_SIZE    1024
#define WEBSOCKET_LOG_MSG_MAX_SIZE    512

static MessageBufferHandle_t logging_message_buffer_handle; /**< Message buffer for logging messages */
static TaskHandle_t logging_task_handle;                    /**< Task handle for the logging task */

static int websocket_logger_func(const char *fmt, va_list args); /**< Websocket logger function */
static void websocket_logger_task(void *pvParameters);           /**< Websocket logger task. */

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

static esp_err_t log_ws_handler(httpd_req_t *req)
{
    webserver_ctx_t *webserver_ctx = (webserver_ctx_t *)req->user_ctx;

    /* Open The websocket connection. */
    if (req->method == HTTP_GET) {

        if (xSemaphoreTake(webserver_ctx->ws_fd_list_mutex, 100 / portTICK_PERIOD_MS) != pdTRUE) {
            ESP_LOGE(TAG, "Failed to take websocket fd list mutex");
            return ESP_FAIL;
        }

        int new_ws = httpd_req_to_sockfd(req);

        ws_list_node_t *ws_list_node = malloc(sizeof(ws_list_node_t));
        ESP_RETURN_ON_FALSE(ws_list_node != NULL, ESP_ERR_NO_MEM, TAG, "Failed to allocate memory for socket.");

        /* Save the socket fd in the list node. */
        ws_list_node->socket_fd = new_ws;

        SLIST_INSERT_HEAD(&webserver_ctx->ws_fd_list, ws_list_node, nodes);

        ESP_LOGI(TAG, "New websocket connection opened. (fd:%d)", new_ws);
        xSemaphoreGive(webserver_ctx->ws_fd_list_mutex);
    }

    return ESP_OK;
}

static httpd_uri_t ws_uri = {.uri = "/log", .method = HTTP_GET, .is_websocket = true, .handler = log_ws_handler};

//-----------------------------------------------------------------------------------------------------------------------

esp_err_t webserver_init(webserver_ctx_t *webserver_ctx)
{
    /* Validate webserver handle. */
    ESP_RETURN_ON_FALSE(webserver_ctx != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid webserver handle.");

    /* Configure HTTP server. */
    httpd_config_t config   = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 20;
    config.server_port      = 80;

    SLIST_INIT(&webserver_ctx->ws_fd_list);

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
    ws_uri.user_ctx = webserver_ctx;
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(webserver_ctx->server, &ws_uri), TAG,
                        "Failed to register websocket handler.");

    /* Redirect logging to websocket. */
    logging_message_buffer_handle = xMessageBufferCreate(WEBSOCKET_LOG_MSG_BUF_SIZE);
    ESP_RETURN_ON_FALSE(logging_message_buffer_handle != NULL, ESP_ERR_NO_MEM, TAG,
                        "Failed to create logging message buffer");
    webserver_ctx->ws_fd_list_mutex = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(webserver_ctx->ws_fd_list_mutex != NULL, ESP_ERR_NO_MEM, TAG,
                        "Failed to create websocket fd list mutex");
    xTaskCreate(websocket_logger_task, "WS_LOGGER", WEBSOCKET_LOG_TASK_STACK_SIZE, webserver_ctx,
                WEBSOCKET_LOG_TASK_PRIO, &logging_task_handle);
    ESP_RETURN_ON_FALSE(logging_task_handle != NULL, ESP_ERR_NO_MEM, TAG, "Failed to create logging task");
    esp_log_set_vprintf(websocket_logger_func);

    return ESP_OK;
}

/*********************************************************************************************************************/
/**                                                PRIVATE FUNCTIONS                                                **/
/*********************************************************************************************************************/

/**
 * @brief Websocket logger function
 * Function is called by the ESP logging library to log messages.
 * Messages are truncated if they exceed WEBSOCKET_LOG_MSG_MAX_SIZE.
 * Messages are print to UART and sent to the logging task over a message buffer.
 *
 * @param fmt Format string (printf style).
 * @param args Variable argument list.
 *
 * @return int Number of characters written, or a negative value if an error occurred.
 */
static int websocket_logger_func(const char *fmt, va_list args)
{

    char log_msg[WEBSOCKET_LOG_MSG_MAX_SIZE] = {0};
    int log_msg_len                          = vsnprintf(log_msg, sizeof(log_msg) - 1, fmt, args);

    if (log_msg_len < 0) {
        printf("Error formatting log message\n");
        return (log_msg_len);
    } else if (log_msg_len >= sizeof(log_msg) - 1) {
        /* Message is to long and will be truncated. */
        log_msg_len = sizeof(log_msg) - 1;
        strcpy(&log_msg[sizeof(log_msg) - 5], "...\n");
        ESP_LOGW(TAG, "Log message truncated");
    }

    /* Print the message to UART. */
    printf(log_msg);

    /* Send from task or ISR context appropriately; drop if buffer is full. */
    if (xPortInIsrContext()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        (void)xMessageBufferSendFromISR(logging_message_buffer_handle, log_msg, (size_t)log_msg_len,
                                        &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken == pdTRUE) {
            portYIELD_FROM_ISR();
        }
    } else {
        (void)xMessageBufferSend(logging_message_buffer_handle, log_msg, (size_t)log_msg_len, 0);
    }
    return log_msg_len;
}

//---------------------------------------------------------------------------------------------------------------------

/**
 * @brief Websocket logger task
 * Task reads messages from a message buffer and sends them to all connected websocket clients.
 * If a send fails, the corresponding websocket client is removed from the list.
 *
 * @param pvParameters Pointer to the webserver context (webserver_ctx_t).
 */
static void websocket_logger_task(void *pvParameters)
{
    webserver_ctx_t *webserver_ctx = (webserver_ctx_t *)pvParameters;

    /* Start logger task. */
    while (1) {
        char log_msg[WEBSOCKET_LOG_MSG_MAX_SIZE] = {0};

        size_t log_msg_len =
            xMessageBufferReceive(logging_message_buffer_handle, &log_msg, WEBSOCKET_LOG_MSG_MAX_SIZE, portMAX_DELAY);
        if (log_msg_len > 0) {
            httpd_ws_frame_t frame = {
                .final      = true,
                .fragmented = false,
                .type       = HTTPD_WS_TYPE_TEXT,
                .payload    = (uint8_t *)log_msg,
                .len        = log_msg_len,
            };

            if (xSemaphoreTake(webserver_ctx->ws_fd_list_mutex, 10 / portTICK_PERIOD_MS) == pdTRUE) {
                ws_list_node_t *node, *tmp;
                SLIST_FOREACH_SAFE(node, &webserver_ctx->ws_fd_list, nodes, tmp)
                {
                    if (httpd_ws_get_fd_info(webserver_ctx->server, node->socket_fd) == HTTPD_WS_CLIENT_WEBSOCKET &&
                        httpd_ws_send_frame_async(webserver_ctx->server, node->socket_fd, &frame) != ESP_OK) {
                        /* Avoid ESP_LOG* here to prevent recursion into the vprintf hook. */
                        ESP_LOGW(TAG, "WS send failed, removing socket (fd:%d)\n", node->socket_fd);
                        SLIST_REMOVE(&webserver_ctx->ws_fd_list, node, ws_list_node_tag, nodes);
                        free(node);
                    }
                }
                xSemaphoreGive(webserver_ctx->ws_fd_list_mutex);
            }
        }
    }
}