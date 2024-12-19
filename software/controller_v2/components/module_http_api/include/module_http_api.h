#pragma once

#include "display.h"
#include "esp_err.h"
#include "module.h"
#include "webserver.h"

/**
 * \brief Initialize the module HTTP API.
 *
 * \param[in] webserver_ctx The webserver context.
 * \param[in] display The display witch contains the modules we want to controle.
 */
esp_err_t module_http_api_init(webserver_ctx_t *webserver_ctx, display_t *display);