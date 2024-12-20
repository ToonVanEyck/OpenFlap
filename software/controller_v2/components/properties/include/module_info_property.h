#pragma once

#include "properties_common.h"

/**
 * \brief Convert the property into it's json representation.
 *
 * \param[out] json The json object in which we will store the property.
 * \param[in] property The property to convert.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static inline esp_err_t module_info_to_json(cJSON **json, const void *property)
{
    assert(json != NULL);
    assert(property != NULL);

    const module_info_property_t *module_info = (const module_info_property_t *)property;

    cJSON_AddBoolToObject(*json, "column_end", module_info->field.column_end);
    cJSON_AddStringToObject(*json, "type", chain_comm_module_type_name_get(module_info->field.type));

    return ESP_OK;
}

/**
 * \brief Deserialize a byte array into a property.
 *
 * \param[out] property The property to populate.
 * \param[in] bin The byte array to deserialize.
 * \param[in] index The index of in case of a multipart property.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static inline esp_err_t module_info_from_binary(void *property, const uint8_t *bin, uint8_t index)
{
    assert(property != NULL);
    assert(bin != NULL);

    module_info_property_t *module_info = (module_info_property_t *)property;

    module_info->raw = bin[0];

    return ESP_OK;
}

/**
 * The module_info property handler.
 *
 * The module_info property is used to send module_infos to the modules. Commands are executed in the modules and do not
 * store any data, as such the property is write-only.
 */
static const property_handler_t PROPERTY_HANDLER_MODULE_INFO = {
    .id = PROPERTY_MODULE_INFO,
    // .name        = "module_info",
    .to_json     = module_info_to_json,
    .from_binary = module_info_from_binary,
    // .from_binary_attributes = {.static_size = 1},
};