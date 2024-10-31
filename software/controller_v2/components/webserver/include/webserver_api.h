#pragma once

#include "webserver.h"

typedef esp_err_t (*webserver_api_handler)(httpd_req_t *r);

/**
 * \brief Add an API endpoint to the webserver
 *
 * Add an API endpoint to the webserver. The endpoint will be accessible at /api/<uri>.
 *
 * \param[in] webserver_handle Handle to the webserver
 * \param[in] uri URI of the endpoint
 * \param[in] method HTTP method of the endpoint
 * \param[in] handler Handler function for the endpoint
 * \param[in] user_ctx User context to be passed to the handler
 *
 * \return esp_err_t
 */
esp_err_t webserver_api_endpoint_add(webserver_handle_t webserver_handle, const char *uri, httpd_method_t method,
                                     webserver_api_handler handler, void *user_ctx);