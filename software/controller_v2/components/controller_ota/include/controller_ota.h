#pragma once

#include "esp_err.h"
#include "webserver.h"

/**
 * \brief Initialize OTA for the controller.
 *
 * \param[in] webserver_ctx The webserver context.
 */
esp_err_t controller_ota_init(webserver_ctx_t *webserver_ctx);