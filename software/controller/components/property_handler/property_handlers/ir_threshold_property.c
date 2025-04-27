#include "esp_check.h"
#include "property_handler_common.h"
#include <string.h>

#define PROPERTY_TAG "IR_THRESHOLD_PROPERTY_HANDLER"

/**
 * \brief Populate the property from a json object.
 *
 * \param[in] json The json object to convert.
 * \param[out] module The module containing the property.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static esp_err_t ir_threshold_from_json(module_t *module, const cJSON *json)
{
    assert(module != NULL);
    assert(json != NULL);

    cJSON *lower_json = cJSON_GetObjectItem(json, "lower");
    cJSON *upper_json = cJSON_GetObjectItem(json, "upper");

    ESP_RETURN_ON_FALSE(cJSON_IsNumber(lower_json), ESP_ERR_INVALID_ARG, PROPERTY_TAG, "Expected a number for lower");
    ESP_RETURN_ON_FALSE(cJSON_IsNumber(upper_json), ESP_ERR_INVALID_ARG, PROPERTY_TAG, "Expected a number for upper");

    ESP_RETURN_ON_FALSE(lower_json->valueint >= 0 && lower_json->valueint <= 1024, ESP_ERR_INVALID_ARG, PROPERTY_TAG,
                        "Value lower_json out of range");
    ESP_RETURN_ON_FALSE(upper_json->valueint >= 0 && upper_json->valueint <= 1024, ESP_ERR_INVALID_ARG, PROPERTY_TAG,
                        "Value upper_json out of range");
    ESP_RETURN_ON_FALSE(lower_json->valueint < upper_json->valueint, ESP_ERR_INVALID_ARG, PROPERTY_TAG,
                        "Value lower_json must be less than upper_json");

    module->ir_threshold.lower = lower_json->valueint;
    module->ir_threshold.upper = upper_json->valueint;

    return ESP_OK;
}

/**
 * \brief Convert the property into it's json representation.
 *
 * \param[out] json The json object in which we will store the property.
 * \param[in] module The module containing the property.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static esp_err_t ir_threshold_to_json(cJSON **json, const module_t *module)
{
    assert(json != NULL);
    assert(module != NULL);

    cJSON_AddNumberToObject(*json, "lower", module->ir_threshold.lower);
    cJSON_AddNumberToObject(*json, "upper", module->ir_threshold.upper);

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
static esp_err_t ir_threshold_from_binary(module_t *module, const uint8_t *bin, uint16_t bin_size)
{
    assert(module != NULL);
    assert(bin != NULL);

    module->ir_threshold.lower = (uint16_t)bin[0] << 8 | (uint16_t)bin[1];
    module->ir_threshold.upper = (uint16_t)bin[2] << 8 | (uint16_t)bin[3];

    return ESP_OK;
}

/**
 * \brief Serialize the property into a byte array.
 *
 * \param[out] bin The serialized byte array.
 * \param[out] bin_size The size of the byte array.
 * \param[in] module The module containing the property.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static esp_err_t ir_threshold_to_binary(uint8_t **bin, uint16_t *bin_size, const module_t *module)
{
    assert(bin != NULL);
    assert(bin_size != NULL);
    assert(module != NULL);

    *bin_size = chain_comm_property_write_attributes_get(PROPERTY_IR_THRESHOLD)->static_property_size;

    *bin = malloc(sizeof(*bin_size));
    ESP_RETURN_ON_FALSE(*bin != NULL, ESP_ERR_NO_MEM, PROPERTY_TAG, "Failed to allocate memory");

    (*bin)[0] = module->ir_threshold.lower >> 8;
    (*bin)[1] = module->ir_threshold.lower & 0xFF;
    (*bin)[2] = module->ir_threshold.upper >> 8;
    (*bin)[3] = module->ir_threshold.upper & 0xFF;

    return ESP_OK;
}

/**
 * \brief Compare the properties of two modules.
 *
 * \param[in] module_a The first module to compare.
 * \param[in] module_b The second module to compare.
 *
 * \return true if the properties are the same, false otherwise.
 */
static bool ir_threshold_compare(const module_t *module_a, const module_t *module_b)
{
    assert(module_a != NULL);
    assert(module_b != NULL);

    /* Compare */
    return memcmp(&module_a->ir_threshold, &module_b->ir_threshold, sizeof(ir_threshold_property_t)) == 0;
}

/**
 * The ir_threshold property handler.
 *
 * The ir_threshold property is used to store the ir_threshold data for the modules. The ir_threshold data
 * is used to adjust the ir_threshold between the character set and the encoder postion.
 */
const property_handler_t PROPERTY_HANDLER_IR_THRESHOLD = {
    .id          = PROPERTY_IR_THRESHOLD,
    .from_json   = ir_threshold_from_json,
    .to_json     = ir_threshold_to_json,
    .from_binary = ir_threshold_from_binary,
    .to_binary   = ir_threshold_to_binary,
    .compare     = ir_threshold_compare,
};

#undef PROPERTY_TAG