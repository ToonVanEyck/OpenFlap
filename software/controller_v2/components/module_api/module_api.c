#include "module_api.h"
#include "esp_check.h"
#include "esp_log.h"
#include "module_api_endpoints.h"
#include "module_api_firmware_endpoints.h"

#define TAG "module_api"

#define MODULE_API_URI          "/module"          /**< module endpoint. */
#define MODULE_FIRMWARE_API_URI "/module/firmware" /**< module firmware endpoint. */

//---------------------------------------------------------------------------------------------------------------------

esp_err_t module_api_init(webserver_ctx_t *webserver_ctx, display_t *display)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "Display is NULL");

    webserver_api_method_handlers_t module_api_handlers = {
        .get_handler  = module_api_get_handler,
        .post_handler = module_api_post_handler,
    };

    ESP_RETURN_ON_ERROR(webserver_api_endpoint_add(webserver_ctx, MODULE_API_URI, &module_api_handlers, true, display),
                        TAG, "Failed to add GET handler for %s", MODULE_API_URI);

    /* Create the api endpoints. */
    webserver_api_method_handlers_t module_api_firmware_handlers = {
        .put_handler = module_api_firmware_handler,
    };

    ESP_RETURN_ON_ERROR(webserver_api_endpoint_add(webserver_ctx, MODULE_FIRMWARE_API_URI,
                                                   &module_api_firmware_handlers, true, display),
                        TAG, "Failed to add endpoint for %s", MODULE_FIRMWARE_API_URI);

    return ESP_OK;
}
