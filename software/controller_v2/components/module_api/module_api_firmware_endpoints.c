#include "module_api_firmware_endpoints.h"
#include "chain_comm_abi.h"
#include "display.h"
#include "esp_check.h"
#include "esp_log.h"
#include "module.h"
#include "property_handler_command.h"
#include "property_handler_firmware.h"
#include "webserver.h"

#define TAG "MODULE_FIRMWARE_ENDPOINTS"

static esp_err_t module_firmware_chunk_handler(void *user_ctx, char *data, size_t data_len, size_t data_offset,
                                               size_t total_data_len);

esp_err_t module_api_firmware_handler(httpd_req_t *req)
{
    display_t *display = (display_t *)req->user_ctx;

    const chain_comm_binary_attributes_t *firmware_write_attr =
        chain_comm_property_write_attributes_get(PROPERTY_FIRMWARE);

    /* Get the chunk size. The chain comm message prefixes 2 bytes to send the address of the send flash chunk.  */
    size_t chunk_size = firmware_write_attr->static_property_size - 2;

    /* Use the webserver_api_util to read and handle the file in chunks.  */
    return webserver_api_util_file_upload_to_chunk_cb(req, module_firmware_chunk_handler, display, chunk_size);
}

//---------------------------------------------------------------------------------------------------------------------

static esp_err_t module_firmware_chunk_handler(void *user_ctx, char *data, size_t data_len, size_t data_offset,
                                               size_t total_data_len)
{
    display_t *display = (display_t *)user_ctx;

    ESP_LOGI(TAG, "writing %d %d/%d bytes", data_len, data_offset + data_len, total_data_len);

    /* Check display size. */
    uint16_t display_size = display_size_get(display);
    ESP_RETURN_ON_FALSE(display_size > 0, ESP_FAIL, TAG, "Display is empty");

    for (uint16_t i = 0; i < display_size; i++) {
        module_t *module = display_module_get(display, i);

        /* Set the firmware page. */
        property_handler_firmware_set(module, data_offset / data_len, (uint8_t *)data, data_len);

        /* Indicate that the firmware property has changed and needs to be written. */
        module_property_indicate_desynchronized(module, PROPERTY_FIRMWARE);

        /* All data has been transmitted, reboot the modules. */
        if (data_offset + data_len == total_data_len) {
            ESP_LOGI(TAG, "Module OTA complete. Rebooting modules...");
            property_handler_command_set(module, CMD_REBOOT);
            module_property_indicate_desynchronized(module, PROPERTY_COMMAND);
        }
    }

    /* Notify that we have updated the display modules. */
    display_event_desynchronized(display);
    /* Wait for synchronisation event. */
    return display_event_wait_for_synchronized(display, pdMS_TO_TICKS(5000));
}