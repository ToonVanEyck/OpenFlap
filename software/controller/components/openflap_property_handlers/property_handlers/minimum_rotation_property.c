#include "esp_check.h"
#include "openflap_property_handler.h"

#define PROPERTY_TAG "MINIMUM_ROTATION_PROPERTY_HANDLER"

/**
 * \brief Populate the property from a json object.
 *
 * \param[in] json The json object to convert.
 * \param[out] module The module containing the property.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static esp_err_t minimum_rotation_from_json(module_t *module, const cJSON *json)
{
    assert(module != NULL);
    assert(json != NULL);

    ESP_RETURN_ON_FALSE(cJSON_IsNumber(json), ESP_ERR_INVALID_ARG, PROPERTY_TAG,
                        "Expected a number for minimum_rotation");

    ESP_RETURN_ON_FALSE(json->valueint >= 0 && json->valueint <= 255, ESP_ERR_INVALID_ARG, PROPERTY_TAG,
                        "Value out of range");

    module->minimum_rotation = json->valueint;

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
static esp_err_t minimum_rotation_to_json(cJSON **json, const module_t *module)
{
    assert(json != NULL);
    assert(module != NULL);

    *json = cJSON_CreateNumber(module->minimum_rotation);

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
static esp_err_t minimum_rotation_from_binary(module_t *module, const uint8_t *bin, uint16_t bin_size)
{
    assert(module != NULL);
    assert(bin != NULL);

    module->minimum_rotation = bin[0];

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
static esp_err_t minimum_rotation_to_binary(uint8_t **bin, uint16_t *bin_size, const module_t *module)
{
    assert(bin != NULL);
    assert(bin_size != NULL);
    assert(module != NULL);

    *bin_size = chain_comm_property_write_attributes_get(PROPERTY_MINIMUM_ROTATION)->static_property_size;

    *bin = malloc(*bin_size);
    ESP_RETURN_ON_FALSE(*bin != NULL, ESP_ERR_NO_MEM, PROPERTY_TAG, "Failed to allocate memory");

    (*bin)[0] = module->minimum_rotation;

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
static bool minimum_rotation_compare(const module_t *module_a, const module_t *module_b)
{
    assert(module_a != NULL);
    assert(module_b != NULL);

    /* Compare */
    return module_a->minimum_rotation == module_b->minimum_rotation;
}

/**
 * The minimum_rotation property handler.
 *
 * The minimum_rotation property is used to store the minimum_rotation data for the modules. The minimum_rotation data
 * is used to adjust the minimum_rotation between the character set and the encoder position.
 */
const property_handler_t PROPERTY_HANDLER_MINIMUM_ROTATION = {
    .id          = PROPERTY_MINIMUM_ROTATION,
    .from_json   = minimum_rotation_from_json,
    .to_json     = minimum_rotation_to_json,
    .from_binary = minimum_rotation_from_binary,
    .to_binary   = minimum_rotation_to_binary,
    .compare     = minimum_rotation_compare,
};

#undef PROPERTY_TAG