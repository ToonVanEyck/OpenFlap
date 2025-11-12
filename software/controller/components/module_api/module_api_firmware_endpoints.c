#include "module_api_firmware_endpoints.h"
#include "esp_check.h"
#include "esp_log.h"
#include "openflap_display.h"
#include "openflap_module.h"
#include "openflap_properties.h"
#include "webserver.h"

#define TAG "MODULE_FIRMWARE_ENDPOINTS"

static esp_err_t module_firmware_chunk_handler(void *user_ctx, char *data, size_t data_len, size_t data_offset,
                                               size_t total_data_len);

//---------------------------------------------------------------------------------------------------------------------

esp_err_t module_api_firmware_handler(httpd_req_t *req)
{
    of_display_t *display = (of_display_t *)req->user_ctx;

    /* Get the chunk size. The chain comm message prefixes 2 bytes to send the address of the send flash chunk.  */
    size_t chunk_size = OF_FIRMWARE_UPDATE_PAGE_SIZE;

    /* Use the webserver_api_util to read and handle the file in chunks.  */
    return webserver_api_util_file_upload_to_chunk_cb(req, module_firmware_chunk_handler, display, chunk_size);
}

//---------------------------------------------------------------------------------------------------------------------

static esp_err_t module_firmware_chunk_handler(void *user_ctx, char *data, size_t data_len, size_t data_offset,
                                               size_t total_data_len)
{
    of_display_t *display = (of_display_t *)user_ctx;

    ESP_LOGI(TAG, "writing %d %d/%d bytes", data_len, data_offset + data_len, total_data_len);

    /* Check display size. */
    uint16_t display_size = display_size_get(display);
    ESP_RETURN_ON_FALSE(display_size > 0, ESP_FAIL, TAG, "Display is empty");

    for (uint16_t i = 0; i < display_size; i++) {
        module_t *module = display_module_get(display, i);

        /* Set the firmware page. */
        of_module_firmware_update_property_set(module, data_offset / data_len, (uint8_t *)data);

        /* Indicate that the firmware property has changed and needs to be written. */
        module_property_indicate_desynchronized(module, OF_MDL_PROP_FIRMWARE_UPDATE);
    }
    display_property_indicate_desynchronized(display, OF_MDL_PROP_FIRMWARE_UPDATE, PROPERTY_SYNC_METHOD_WRITE);

    /* Synchronize. */
    ESP_RETURN_ON_ERROR(of_display_synchronize(display, 5000), TAG, "Failed to synchronize display.");

    if (data_offset + data_len == total_data_len) {
        ESP_LOGI(TAG, "Module OTA complete. Rebooting modules...");

        for (uint16_t i = 0; i < display_size; i++) {
            module_t *module = display_module_get(display, i);
            /* All data has been transmitted, reboot the modules. */
            of_module_command_set(module, CMD_REBOOT);
            module_property_indicate_desynchronized(module, OF_MDL_PROP_COMMAND);
        }
        display_property_indicate_desynchronized(display, OF_MDL_PROP_COMMAND, PROPERTY_SYNC_METHOD_WRITE);
        /* Synchronize. */
        ESP_RETURN_ON_ERROR(of_display_synchronize(display, 5000), TAG, "Failed to synchronize display.");
    }

    return ESP_OK;
}