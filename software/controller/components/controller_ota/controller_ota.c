#include "controller_ota.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#define TAG "CONTROLLER_OTA"

/** OTA chunk size. The firmware PUT request is read and handled in chunks of this size.  */
#define CONTROLLER_OTA_CHUNK_SIZE (2048)

#define CONTROLLER_OTA_URI "/firmware/controller" /**< Controller OTA endpoint. */

static esp_err_t controller_ota_handler(httpd_req_t *req);
static esp_err_t controller_ota_chunk_handler(void *ctx, char *data, size_t data_len, size_t data_offset,
                                              size_t total_data_len);
static void controller_ota_reboot_callback(TimerHandle_t xTimer);

//---------------------------------------------------------------------------------------------------------------------

esp_err_t controller_ota_init(controller_ota_ctx_t *ctx, webserver_ctx_t *webserver_ctx)
{
    ESP_RETURN_ON_FALSE(ctx != NULL, ESP_ERR_INVALID_ARG, TAG, "Controller OTA context is NULL");

    /* Clear the context. */
    memset(ctx, 0, sizeof(controller_ota_ctx_t));

    /* Create the api endpoints. */
    webserver_api_method_handlers_t controller_ota_handlers = {
        .put_handler = controller_ota_handler,
    };

    ESP_RETURN_ON_ERROR(
        webserver_api_endpoint_add(webserver_ctx, CONTROLLER_OTA_URI, &controller_ota_handlers, true, ctx), TAG,
        "Failed to add endpoint for %s", CONTROLLER_OTA_URI);

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t controller_ota_verify_firmware(bool startup_success)
{
    /* Get the running partition. */
    const esp_partition_t *running_partition = esp_ota_get_running_partition();

    /* Get the partition state. */
    esp_ota_img_states_t ota_state;
    ESP_RETURN_ON_ERROR(esp_ota_get_state_partition(running_partition, &ota_state), TAG,
                        "Failed to get partition state");

    if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
        if (startup_success) {
            esp_ota_mark_app_valid_cancel_rollback();
        } else {
            ESP_LOGE(TAG, "Application did not start successfully, rolling back to the previous version ...");
            esp_ota_mark_app_invalid_rollback_and_reboot();
        }
    } else if (!startup_success) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Handler for the controller OTA endpoint.
 *
 * \param[in] req The HTTP request.
 *
 * \return esp_err_t
 */
static esp_err_t controller_ota_handler(httpd_req_t *req)
{
    controller_ota_ctx_t *ctx = (controller_ota_ctx_t *)req->user_ctx;

    /* Use the webserver_api_util to read and handle the file in chunks.  */
    return webserver_api_util_file_upload_to_chunk_cb(req, controller_ota_chunk_handler, ctx,
                                                      CONTROLLER_OTA_CHUNK_SIZE);
}

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Handler for a chunk of data provided by the controller OTA endpoint.
 *
 * The data is written to the OTA partition in chunks. This is done because there is not enough memory to receive and
 * store the entire binary before writing it to the partition.
 *
 * \param[in] user_ctx The OTA context.
 * \param[in] data The data chunk.
 * \param[in] data_len The length of the data chunk.
 * \param[in] data_offset The offset of the data chunk relative to the total data.
 * \param[in] total_data_len The total length of the data.
 *
 * \return esp_err_t
 */
static esp_err_t controller_ota_chunk_handler(void *user_ctx, char *data, size_t data_len, size_t data_offset,
                                              size_t total_data_len)
{
    ESP_RETURN_ON_FALSE(user_ctx != NULL, ESP_ERR_INVALID_ARG, TAG, "Controller OTA context is NULL");
    controller_ota_ctx_t *ctx = (controller_ota_ctx_t *)user_ctx;

    esp_err_t ret = ESP_OK; /* Set by ESP_GOTO_ON_ERROR macro. */

    /* Start OTA. */
    if (data_offset == 0) {
        ESP_LOGI(TAG, "Starting controller OTA.");
        /* Get the next updatable OTA partition and start OTA. */
        ctx->update_partition = esp_ota_get_next_update_partition(NULL);
        ESP_RETURN_ON_FALSE(ctx->update_partition != NULL, ESP_FAIL, TAG, "Failed to get next update partition");
        ESP_GOTO_ON_ERROR(esp_ota_begin(ctx->update_partition, total_data_len, &ctx->update_handle), exit_abort, TAG,
                          "Failed to begin OTA update");
    }

    /* Write OTA data. */
    ESP_LOGI(TAG, "writing %d %d/%d bytes", data_len, data_offset + data_len, total_data_len);
    ESP_GOTO_ON_ERROR(esp_ota_write(ctx->update_handle, data, data_len), exit_abort, TAG, "Failed to write OTA data");

    /* Finalize OTA. */
    if (data_offset + data_len == total_data_len) {
        /* End OTA and mark the partition for next boot. */
        ESP_GOTO_ON_ERROR(esp_ota_end(ctx->update_handle), exit_abort, TAG, "Failed to end OTA update");
        ESP_RETURN_ON_ERROR(esp_ota_set_boot_partition(ctx->update_partition), TAG, "Failed to set boot partition");
        ESP_LOGI(TAG, "Controller OTA complete. Rebooting in 3 seconds...");

        /* Start a FreeRTOS timer to reboot the device in 3 seconds. */
        TimerHandle_t reboot_timer =
            xTimerCreate("reboot_timer", pdMS_TO_TICKS(3000), pdFALSE, NULL, controller_ota_reboot_callback);
        if (reboot_timer != NULL) {
            xTimerStart(reboot_timer, 0);
        } else {
            ESP_LOGE(TAG, "Failed to create reboot timer.");
        }
    }

exit_abort:
    /* Abort the ota if an error occurred. */
    if (ret != ESP_OK) {
        esp_ota_abort(ctx->update_handle);
        ESP_LOGE(TAG, "Controller OTA failed.");
    }

    return ret;
}

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Callback to reboot the device.
 *
 * \param[in] xTimer The timer handle.
 */
static void controller_ota_reboot_callback(TimerHandle_t xTimer)
{
    esp_restart();
}