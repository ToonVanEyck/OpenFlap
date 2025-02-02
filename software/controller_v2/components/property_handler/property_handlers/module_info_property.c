#include "property_handler_common.h"

#define PROPERTY_TAG "MODULE_INFO_PROPERTY_HANDLER"

/**
 * \brief Convert the property into it's json representation.
 *
 * \param[out] json The json object in which we will store the property.
 * \param[in] module The module containing the property.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static esp_err_t module_info_to_json(cJSON **json, const module_t *module)
{
    assert(json != NULL);
    assert(module != NULL);

    const module_info_property_t *module_info = &module->module_info;

    cJSON_AddBoolToObject(*json, "column_end", module_info->column_end);
    cJSON_AddStringToObject(*json, "type", chain_comm_module_type_name_get(module_info->type));

    return ESP_OK;
}

/**
 * \brief Deserialize a byte array into a property.
 *
 * \param[out] module The module containing the property.
 * \param[in] bin The byte array to deserialize.
 * \param[in] bin_size The size of the byte array.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static esp_err_t module_info_from_binary(module_t *module, const uint8_t *bin, uint16_t bin_size)
{
    assert(module != NULL);
    assert(bin != NULL);

    module_info_property_t *module_info = &module->module_info;

    module_info->raw = bin[0];

    return ESP_OK;
}

/**
 * The module_info property handler.
 *
 * The module_info property is used to send module_infos to the modules. Commands are executed in the modules and do not
 * store any data, as such the property is write-only.
 */
const property_handler_t PROPERTY_HANDLER_MODULE_INFO = {
    .id          = PROPERTY_MODULE_INFO,
    .to_json     = module_info_to_json,
    .from_binary = module_info_from_binary,
};

#undef PROPERTY_TAG