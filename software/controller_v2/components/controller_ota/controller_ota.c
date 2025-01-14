#include "controller_ota.h"
#include "esp_check.h"
#include "esp_log.h"

#define TAG "CONTROLLER_OTA"

esp_err_t controller_ota_post_handler(httpd_req_t *req);

//---------------------------------------------------------------------------------------------------------------------

esp_err_t controller_ota_init(webserver_ctx_t *webserver_ctx)
{
    ESP_RETURN_ON_ERROR(
        webserver_api_endpoint_add(webserver_ctx, "/module/firmware", HTTP_POST, controller_ota_post_handler, NULL),
        TAG, "Failed to add POST handler for /module/firmware");
    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t controller_ota_post_handler(httpd_req_t *req)
{
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}