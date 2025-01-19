#pragma once

#include "esp_err.h"
#include "esp_ota_ops.h"
#include "webserver.h"

typedef struct {
    const esp_partition_t *update_partition; /**< Update partition. */
    esp_ota_handle_t update_handle;          /**< Update handle. */
} controller_ota_ctx_t;

/**
 * \brief Initialize OTA for the controller.
 *
 * \param[in] ctx The OTA context.
 * \param[in] webserver_ctx The webserver context.
 *
 * \return esp_err_t
 */
esp_err_t controller_ota_init(controller_ota_ctx_t *ctx, webserver_ctx_t *webserver_ctx);
esp_err_t controller_ota_verify_firmware(bool startup_success);