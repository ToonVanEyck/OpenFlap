#include "module_http_api.h"
#include "cJSON.h"
#include "esp_check.h"
#include "esp_log.h"

#define TAG "MODULE_HTTP_API"

esp_err_t module_http_api_get_handler(httpd_req_t *req);
esp_err_t module_http_api_post_handler(httpd_req_t *req);

//---------------------------------------------------------------------------------------------------------------------

esp_err_t module_http_api_init(webserver_ctx_t *webserver_ctx, display_t *display)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "Display is NULL");

    ESP_RETURN_ON_ERROR(
        webserver_api_endpoint_add(webserver_ctx, "/module", HTTP_GET, module_http_api_get_handler, display), TAG,
        "Failed to add GET handler for /module");
    ESP_RETURN_ON_ERROR(
        webserver_api_endpoint_add(webserver_ctx, "/module", HTTP_POST, module_http_api_post_handler, display), TAG,
        "Failed to add POST handler for /module");
    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t module_http_api_get_handler(httpd_req_t *req)
{
    display_t *display = (display_t *)req->user_ctx;
    (void)display;
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "module", "placeholder");
    char *json_str = cJSON_Print(json);
    cJSON_Delete(json);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    free(json_str);

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t module_http_api_post_handler(httpd_req_t *req)
{
    display_t *display = (display_t *)req->user_ctx;
    ESP_LOGI(TAG, "POST data length: %d", req->content_len);

    /* Attempt to allocate memory for posted data. */
    char *buf = malloc(req->content_len);
    if (buf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for POST data");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, NULL);
        return ESP_FAIL;
    }

    /* Read All data. */
    for (int total_len = 0, recv_len = 0; total_len < req->content_len; total_len += recv_len) {
        recv_len = httpd_req_recv(req, buf + total_len, req->content_len - total_len);
        if (recv_len <= 0) {
            ESP_LOGE(TAG, "Failed to receive POST data");
            free(buf);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, NULL);
            return ESP_FAIL;
        }
    }

    /* Convert to json. */
    cJSON *json = cJSON_ParseWithLength(buf, req->content_len);
    if (json == NULL) {
        ESP_LOGE(TAG, "Failed to parse POST data");
        free(buf);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, NULL);
        return ESP_FAIL;
    }

    /* Check that root element is an array. */
    if (!cJSON_IsArray(json)) {
        ESP_LOGE(TAG, "POST data is not an object");
        cJSON_Delete(json);
        free(buf);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL);
        return ESP_FAIL;
    }

    /* Loop through array. */
    cJSON *module_json = NULL;
    cJSON_ArrayForEach(module_json, json)
    {
        // char *module_json_str = cJSON_Print(module_json);
        // if (module_json_str) {
        //     ESP_LOGI(TAG, "Module JSON: %s", module_json_str);
        //     free(module_json_str);
        // }
        /* Check if module has valid index specified. */
        cJSON *module_index_json = cJSON_GetObjectItem(module_json, "module");
        if (!cJSON_IsNumber(module_index_json)) {
            ESP_LOGE(TAG, "No module index specified");
            continue;
        }

        /* Update the module. */
        module_t *module = display_module_get(display, module_index_json->valueint);
        if (module) {
            module_properties_set_from_json(module, module_json);
        }
    }

    /* Gracefully exit. */
    cJSON_Delete(json);
    free(buf);
    httpd_resp_sendstr(req, "OK");

    /* Notify that we have updated the display modules. */
    display_event_desynchronized(display);

    return ESP_OK;
}