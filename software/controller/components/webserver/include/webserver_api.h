#pragma once

#include "webserver_common.h"
#include <stdbool.h>

typedef esp_err_t (*webserver_api_handler)(httpd_req_t *req);

/**
 * \brief callback function to use for handling chunked data.
 *
 * \param[in] data A chunk of data.
 * \param[in] data_len The length of the current data chuck.
 * \param[in] data_offset The offset of the current data chunk.
 * \param[in] total_data_len The total length of the data.
 *
 * \return esp_err_t
 */
typedef esp_err_t (*webserver_api_util_chunk_handler)(void *user_ctx, char *data, size_t data_len, size_t data_offset,
                                                      size_t total_data_len);

typedef struct {
    webserver_api_handler get_handler;
    webserver_api_handler post_handler;
    webserver_api_handler put_handler;
    webserver_api_handler delete_handler;
    webserver_api_handler options_handler;
} webserver_api_method_handlers_t;

/**
 * \brief Add an API endpoint to the webserver
 *
 * Add an API endpoint to the webserver. The endpoint will be accessible at /api/<uri>.
 *
 * \param[in] webserver_ctx Handle to the webserver.
 * \param[in] uri URI of the endpoint.
 * \param[in] handlers Handler functions for the different HTTP methods.
 * \param[in] allow_cors Allow CORS for this endpoint.
 * \param[in] user_ctx User context to be passed to the handler.
 *
 * \return esp_err_t
 */
esp_err_t webserver_api_endpoint_add(webserver_ctx_t *webserver_ctx, const char *uri,
                                     webserver_api_method_handlers_t *handlers, bool allow_cors, void *user_ctx);

/**
 * \brief A utility function to handle file uploads by reading the file in chunks and calling a callback for each chunk.
 *
 * \param[in] req HTTP request object.
 * \param[in] chunk_handler The chunk handler callback function.
 * \param[in] chunk_size The size of the chunks to read.
 *
 * \retval ESP_OK File was uploaded successfully.
 * \retval ESP_FAIL Failed to upload file.
 * \retval ESP_ERR_TIMEOUT Upload timed out.
 * \retval ESP_ERR_NO_MEM Memory allocation failed.
 */
esp_err_t webserver_api_util_file_upload_to_chunk_cb(httpd_req_t *req, webserver_api_util_chunk_handler chunk_handler,
                                                     void *user_ctx, size_t chunk_size);
