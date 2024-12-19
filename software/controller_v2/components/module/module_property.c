#include "module_property.h"
#include "cJSON.h"
#include "esp_check.h"
#include "esp_log.h"
#include "properties.h"

#include <string.h>

#define TAG "MODULE_PROPERTY"

void *module_property_get_by_id(module_t *module, property_id_t property_id)
{
    if (module == NULL) {
        ESP_LOGE(TAG, "Module is NULL");
        return NULL;
    }

    switch (property_id) {
        case PROPERTY_MODULE_INFO:
            return &module->module_info;
        case PROPERTY_COMMAND:
            return &module->command;
        default:
            switch (MODULE_INFO_TYPE(module->module_info)) {
                case MODULE_TYPE_SPLITFLAP:
                    return module_splitflap_property_get_by_id(&module->splitflap, property_id);
                default:
                    return NULL;
            }
    }
}

//---------------------------------------------------------------------------------------------------------------------

void *module_property_get_by_name(module_t *module, const char *property_name)
{
    if (module == NULL) {
        ESP_LOGE(TAG, "Module is NULL");
        return NULL;
    }

    const property_handler_t *property_handler = property_handler_get_by_name(property_name);

    if (property_handler == NULL) {
        return NULL;
    }

    return module_property_get_by_id(module, property_handler->id);
}

//---------------------------------------------------------------------------------------------------------------------

uint8_t module_properties_set_from_json(module_t *module, const cJSON *json)
{
    /* Validate inputs. */
    if (module == NULL || json == NULL) {
        return 0;
    }

    uint8_t property_ok_cnt = 0;
    cJSON *json_property    = NULL;
    /* Loop through all json object in this json object. */
    cJSON_ArrayForEach(json_property, json)
    {
        /* Ignore the module index. */
        if (strcmp(json_property->string, "module") == 0) {
            continue;
        }

        /* Get the property handler. */
        const property_handler_t *property_handler = property_handler_get_by_name(json_property->string);
        if (property_handler == NULL) {
            ESP_LOGE("MODULE", "Property \"%s\" not supported by controller.", json_property->string);
            continue;
        }

        /* Get property field from the module. */
        void *property = module_property_get_by_id(module, property_handler->id);
        if (property == NULL) {
            ESP_LOGE("MODULE", "Property \"%s\" not supported by module of type %d.", json_property->string,
                     MODULE_INFO_TYPE(module->module_info));
            continue;
        }

        /* Check if the property is writable. */
        if (property_handler->from_json == NULL) {
            ESP_LOGE("MODULE", "Property \"%s\" is not writable.", json_property->string);
            continue;
        }

        /* Call the property handler. */
        if (property_handler->from_json(&property, json_property) == ESP_FAIL) {
            ESP_LOGE("MODULE", "Property \"%s\" is invalid.", json_property->string);
            continue;
        }

        /* Indicate that the property needs to be written to the actual module. */
        module_property_indicate_desynchronized(module, property_handler->id, PROPERTY_SYNC_METHOD_WRITE);

        /* Property has been written. */
        property_ok_cnt++;
        ESP_LOGI("MODULE", "JSON contains property: \"%s\"", json_property->string);
    }
    return property_ok_cnt;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t module_property_indicate_desynchronized(module_t *module, property_id_t property_id,
                                                  property_sync_method_t sync_method)
{
    /* Validate inputs. */
    ESP_RETURN_ON_FALSE(module != NULL, ESP_ERR_INVALID_ARG, TAG, "Module is NULL");
    ESP_RETURN_ON_FALSE(property_id < PROPERTIES_MAX, ESP_ERR_INVALID_ARG, TAG, "Invalid property id");

    /* Indicate that the property has been desynchronized. */
    if (sync_method == PROPERTY_SYNC_METHOD_READ) {
        module->sync_properties_read_required |= (1 << property_id);
        module->sync_properties_write_required &= ~(1 << property_id);
    } else {
        module->sync_properties_write_required |= (1 << property_id);
        module->sync_properties_read_required &= ~(1 << property_id);
    }

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t module_property_indicate_synchronized(module_t *module, property_id_t property_id)
{
    /* Validate inputs. */
    ESP_RETURN_ON_FALSE(module != NULL, ESP_ERR_INVALID_ARG, TAG, "Module is NULL");
    ESP_RETURN_ON_FALSE(property_id < PROPERTIES_MAX, ESP_ERR_INVALID_ARG, TAG, "Invalid property id");

    /* Indicate that the property has been synchronized. */
    module->sync_properties_read_required &= ~(1 << property_id);
    module->sync_properties_write_required &= ~(1 << property_id);

    return ESP_OK;
}