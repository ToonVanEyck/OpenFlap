#include "esp_check.h"
#include "property_handler_common.h"

#define PROPERTY_TAG "CHARACTER_PROPERTY_HANDLER"

/**
 * \brief Populate the property from a json object.
 *
 * \param[in] json The json object to convert.
 * \param[out] module The module containing the property.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static esp_err_t character_from_json(module_t *module, const cJSON *json)
{
    assert(module != NULL);
    assert(json != NULL);

    uint8_t *character_index = &module->character_index;

    ESP_RETURN_ON_FALSE(cJSON_IsString(json), ESP_ERR_INVALID_ARG, PROPERTY_TAG, "Expected a character");

    ESP_RETURN_ON_ERROR(module_character_set_index_of_character(module, character_index, json->valuestring),
                        PROPERTY_TAG, "Character %s not in character set", json->valuestring);

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
static esp_err_t character_to_json(cJSON **json, const module_t *module)
{
    assert(json != NULL);
    assert(module != NULL);

    const uint8_t *character_index = &module->character_index;

    *json = cJSON_CreateString((const char *)&module->character_set->data[4 * (*character_index)]);
    ESP_RETURN_ON_FALSE(*json != NULL, ESP_FAIL, PROPERTY_TAG, "Failed to create JSON string");

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
static esp_err_t character_from_binary(module_t *module, const uint8_t *bin, uint16_t bin_size)
{
    assert(module != NULL);
    assert(bin != NULL);

    uint8_t *character_index = &module->character_index;

    *character_index = bin[0];

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
static esp_err_t character_to_binary(uint8_t **bin, uint16_t *bin_size, const module_t *module)
{
    assert(bin != NULL);
    assert(bin_size != NULL);
    assert(module != NULL);

    const uint8_t *character_index = &module->character_index;

    *bin_size = chain_comm_property_write_attributes_get(PROPERTY_CHARACTER)->static_property_size;

    *bin = malloc(*bin_size);
    ESP_RETURN_ON_FALSE(*bin != NULL, ESP_ERR_NO_MEM, PROPERTY_TAG, "Failed to allocate memory");

    (*bin)[0] = *character_index;

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
static bool character_compare(const module_t *module_a, const module_t *module_b)
{
    assert(module_a != NULL);
    assert(module_b != NULL);

    const uint8_t *character_index_a = &module_a->character_index;
    const uint8_t *character_index_b = &module_b->character_index;

    /* Compare */
    return *character_index_a == *character_index_b;
}

/**
 * The character property handler.
 *
 * The character property is used to store the character data for the modules.
 */
const property_handler_t PROPERTY_HANDLER_CHARACTER = {
    .id          = PROPERTY_CHARACTER,
    .from_json   = character_from_json,
    .to_json     = character_to_json,
    .from_binary = character_from_binary,
    .to_binary   = character_to_binary,
    .compare     = character_compare,
};

#undef PROPERTY_TAG