#include "openflap_property_handler.h"

#include "cJSON.h"
#include "esp_check.h"
#include "esp_log.h"
#include "module_api_endpoints.h"
#include "openflap_display.h"
#include "openflap_module.h"
#include "openflap_properties.h"

#define MODULE_INDEX_KEY_STR "module"

#define TAG "MODULE_ENDPOINTS"

esp_err_t module_api_get_handler(httpd_req_t *req)
{
    of_display_t *display = (of_display_t *)req->user_ctx;
    esp_err_t err         = ESP_OK;

    /* Desynchronize all properties which can be read through json. to force them to be read. */
    for (cc_prop_id_t prop_id = 0; prop_id < OF_CC_PROP_CNT; prop_id++) {
        if (cc_prop_list[prop_id].handler.get_alt != NULL) {
            display_property_indicate_desynchronized(display, prop_id, PROPERTY_SYNC_METHOD_READ);
        }
    }
    ESP_LOGI(TAG, "Display desynchronized");

    /* Synchronize */
    err = of_display_synchronize(display, 5000);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to synchronize display");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, NULL);
        return err;
    }
    ESP_LOGI(TAG, "Display synchronized");
    /* Create a json object. */
    cJSON *json = cJSON_CreateArray();
    for (uint16_t i = 0; i < display_size_get(display); i++) {
        module_t *module   = display_module_get(display, i);
        cJSON *module_json = cJSON_CreateObject();

        cJSON_AddNumberToObject(module_json, MODULE_INDEX_KEY_STR, i);

        /* Get all properties from a module. */
        for (cc_prop_id_t prop_id = 0; prop_id < OF_CC_PROP_CNT; prop_id++) {
            /* Get the property handler. */
            const char *property_name = cc_prop_list[prop_id].attribute.name;

            /* Check if the property can be converted to a JSON. */
            if (cc_prop_list[prop_id].handler.get_alt == NULL) {
                /* Property is not supported for reading. */
                continue;
            }

            /* Call the property handler. */
            cJSON *property_json = cJSON_CreateObject();
            if (!cc_prop_list[prop_id].handler.get_alt(module, i, &property_json)) {
                ESP_LOGE("MODULE", "Property \"%s\" is invalid.", property_name);
                continue;
            }

            /* Add the property json to the module json. */
            cJSON_AddItemToObject(module_json, property_name, property_json);
        }

        /* Add the module json to the array. */
        cJSON_AddItemToArray(json, module_json);
    }
    char *json_str = cJSON_Print(json);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));

    cJSON_Delete(json);
    free(json_str);

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t module_api_post_handler(httpd_req_t *req)
{
    of_display_t *display = (of_display_t *)req->user_ctx;

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
        /* Check if module has valid index specified. */
        cJSON *module_index_json = cJSON_GetObjectItem(module_json, MODULE_INDEX_KEY_STR);
        if (!cJSON_IsNumber(module_index_json)) {
            ESP_LOGE(TAG, "No module index specified");
            continue;
        }

        /* Update the module. */
        module_t *module = display_module_get(display, module_index_json->valueint);
        if (module == NULL) {
            ESP_LOGE(TAG, "Display does not contain module with index %d", module_index_json->valueint);
            continue;
        }

        /* Update all properties. */
        cJSON *property_json = NULL;

        /* Loop through all json object in the module_json object. */
        cJSON_ArrayForEach(property_json, module_json)
        {
            /* Ignore the module index. */
            if (strcmp(property_json->string, "module") == 0) {
                continue;
            }

            /* Get the property handler. */
            cc_prop_id_t prop_id = cc_prop_id_by_name(property_json->string);
            if (prop_id == -1) {
                ESP_LOGE("MODULE", "Property \"%s\" not supported by controller.", property_json->string);
                continue;
            }

            /* Get property field from the module. */
            // module_t *module = module_property_get_by_id(module, property_handler->id);
            // if (property == NULL) {
            //     ESP_LOGE("MODULE", "Property \"%s\" not supported by module of type %d.", property_json->string,
            //              module->module_info.type);
            //     continue;
            // }

            /* Check if the property is writable. */
            if (cc_prop_list[prop_id].handler.set_alt == NULL) {
                ESP_LOGE("MODULE", "Property \"%s\" is not writable.", property_json->string);
                continue;
            }

            /* Call the property handler. */
            if (!cc_prop_list[prop_id].handler.set_alt(module, module_index_json->valueint, property_json)) {
                ESP_LOGE("MODULE", "Property \"%s\" is invalid.", property_json->string);
                continue;
            }

            /* Indicate that the property needs to be written to the actual module. */
            module_property_indicate_desynchronized(module, prop_id);
            display_property_indicate_desynchronized(display, prop_id, PROPERTY_SYNC_METHOD_WRITE);
        }
    }

    /* Gracefully exit. */
    cJSON_Delete(json);
    free(buf);
    httpd_resp_sendstr(req, "OK");

    /* Notify that we have updated the display modules. */
    of_display_synchronize(display, 0);

    return ESP_OK;
}