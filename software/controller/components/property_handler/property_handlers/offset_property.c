#include "esp_check.h"
#include "property_handler_common.h"

#define PROPERTY_TAG "OFFSET_PROPERTY_HANDLER"

/**
 * \brief Populate the property from a json object.
 *
 * \param[in] json The json object to convert.
 * \param[out] module The module containing the property.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static esp_err_t offset_from_json(module_t *module, const cJSON *json)
{
    assert(module != NULL);
    assert(json != NULL);

    ESP_RETURN_ON_FALSE(cJSON_IsNumber(json), ESP_ERR_INVALID_ARG, PROPERTY_TAG, "Expected a number for offset");

    ESP_RETURN_ON_FALSE(json->valueint >= 0 && json->valueint <= 255, ESP_ERR_INVALID_ARG, PROPERTY_TAG,
                        "Value out of range");

    module->offset = json->valueint;

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
static esp_err_t offset_to_json(cJSON **json, const module_t *module)
{
    assert(json != NULL);
    assert(module != NULL);

    *json = cJSON_CreateNumber(module->offset);

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
static esp_err_t offset_from_binary(module_t *module, const uint8_t *bin, uint16_t bin_size)
{
    assert(module != NULL);
    assert(bin != NULL);

    module->offset = bin[0];

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
static esp_err_t offset_to_binary(uint8_t **bin, uint16_t *bin_size, const module_t *module)
{
    assert(bin != NULL);
    assert(bin_size != NULL);
    assert(module != NULL);

    *bin_size = chain_comm_property_write_attributes_get(PROPERTY_OFFSET)->static_property_size;

    *bin = malloc(sizeof(*bin_size));
    ESP_RETURN_ON_FALSE(*bin != NULL, ESP_ERR_NO_MEM, PROPERTY_TAG, "Failed to allocate memory");

    (*bin)[0] = module->offset;

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
static bool offset_compare(const module_t *module_a, const module_t *module_b)
{
    assert(module_a != NULL);
    assert(module_b != NULL);

    /* Compare */
    return module_a->offset == module_b->offset;
}

/**
 * The offset property handler.
 *
 * The offset property is used to store the offset data for the modules. The offset data is used to
 * adjust the offset between the character set and the encoder postion.
 */
const property_handler_t PROPERTY_HANDLER_OFFSET = {
    .id          = PROPERTY_OFFSET,
    .from_json   = offset_from_json,
    .to_json     = offset_to_json,
    .from_binary = offset_from_binary,
    .to_binary   = offset_to_binary,
    .compare     = offset_compare,
};

#undef PROPERTY_TAG