#include "modules.h"
#include "cJSON.h"
#include "esp_log.h"
#include "properties.h"

#include <string.h>

void *module_property_get_by_id(module_t *module, property_id_t property_id)
{
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
        if (property_handler->from_json(&property, json_property) == false) {
            ESP_LOGE("MODULE", "Property \"%s\" is invalid.", json_property->string);
            continue;
        }

        /* Property has been written. */
        property_ok_cnt++;
        ESP_LOGI("MODULE", "JSON contains property: \"%s\"", json_property->string);
    }
    return property_ok_cnt;
}