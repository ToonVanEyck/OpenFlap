#pragma once

#include "esp_http_server.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <sys/queue.h>

typedef struct ws_list_node_tag {
    int socket_fd;                       /**< Socket file descriptor. */
    SLIST_ENTRY(ws_list_node_tag) nodes; /* links to next element */
} ws_list_node_t;

/**
 * @brief Head of linked list.
 */
typedef SLIST_HEAD(ws_list_head_tag, ws_list_node_tag) ws_list_head_t;

/**
 * \brief Context of the webserver
 */
typedef struct {
    httpd_handle_t server;              /**< Handle to the HTTP server */
    ws_list_head_t ws_fd_list;          /**< List of websocket file descriptors */
    SemaphoreHandle_t ws_fd_list_mutex; /**< Mutex to protect the websocket fd list */
} webserver_ctx_t;
