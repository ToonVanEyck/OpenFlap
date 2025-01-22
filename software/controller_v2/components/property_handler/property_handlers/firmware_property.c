#include "esp_check.h"
#include "property_handler_common.h"
#include "property_handler_firmware.h"

#include <string.h>

#define PROPERTY_TAG "FIRMWARE_PROPERTY_HANDLER"

/**
 * \brief Convert the property into it's json representation.
 *
 * \param[out] json The json object in which we will store the property.
 * \param[in] property The property to convert.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static esp_err_t firmware_to_json(cJSON **json, const module_t *module)
{
    assert(json != NULL);
    assert(module != NULL);

    const char *firmware_version = "v0.0.0";

    *json = cJSON_CreateString(firmware_version);
    ESP_RETURN_ON_FALSE(*json != NULL, ESP_FAIL, PROPERTY_TAG, "Failed to create JSON string");

    return ESP_OK;
}

/**
 * \brief Deserialize a byte array into a property.
 *
 * \param[out] property The property to populate.
 * \param[in] bin The byte array to deserialize.
 * \param[in] bin_size The size of the byte array.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static esp_err_t firmware_from_binary(module_t *module, const uint8_t *bin, uint16_t bin_size)
{
    assert(module != NULL);
    assert(bin != NULL);

    return ESP_OK;
}

/**
 * \brief Serialize the property into a byte array.
 *
 * \param[out] bin The serialized byte array.
 * \param[out] bin_size The size of the byte array.
 * \param[in] property The property to serialize.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static esp_err_t firmware_to_binary(uint8_t **bin, uint16_t *bin_size, const module_t *module)
{
    assert(bin != NULL);
    assert(bin_size != NULL);
    assert(module != NULL);

    const firmware_property_t *firmware = &module->firmware;

    /* Check if the character set has been initialized before. */
    ESP_RETURN_ON_FALSE(firmware->data != NULL, ESP_ERR_INVALID_ARG, PROPERTY_TAG, "Character set is NULL");

    *bin_size = chain_comm_property_write_attributes_get(PROPERTY_FIRMWARE)->static_property_size;

    *bin = malloc(*bin_size);
    ESP_RETURN_ON_FALSE(*bin != NULL, ESP_ERR_NO_MEM, PROPERTY_TAG, "Failed to allocate memory");

    /* Copy the index and firmware page to the binary array. */
    (*bin)[0] = (firmware->index >> 8) & 0xFF;
    (*bin)[1] = firmware->index & 0xFF;
    memcpy((*bin) + 2, firmware->data, *bin_size);

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
static bool firmware_compare(const module_t *module_a, const module_t *module_b)
{
    assert(module_a != NULL);
    assert(module_b != NULL);

    const firmware_property_t *firmware_a = &module_a->firmware;
    const firmware_property_t *firmware_b = &module_b->firmware;

    /* Check if the sizes are the same. */
    if (firmware_a->index != firmware_b->index) {
        return false;
    }

    uint16_t size = chain_comm_property_write_attributes_get(PROPERTY_FIRMWARE)->static_property_size - 2;

    /* Compare the character sets. */
    return memcmp(firmware_a->data, firmware_b->data, size) == 0;
}

/**
 * The firmware property handler.
 *
 * The firmware property is used to store the firmware data for the modules.
 */
const property_handler_t PROPERTY_HANDLER_FIRMWARE = {
    .id          = PROPERTY_FIRMWARE,
    .to_json     = firmware_to_json,
    .from_binary = NULL,
    .to_binary   = firmware_to_binary,
    .compare     = firmware_compare,
};

//----------------------------------------------------------------------------------------------------------------------------------

esp_err_t property_handler_firmware_set(module_t *module, uint16_t index, const uint8_t *data, uint16_t data_size)
{
    assert(module != NULL);
    assert(data != NULL);

    firmware_property_t *firmware = &module->firmware;

    /* Check if the character set has been initialized before. */
    ESP_RETURN_ON_FALSE(firmware->data != NULL, ESP_ERR_INVALID_ARG, PROPERTY_TAG, "Character set is NULL");

    /* Copy the index and firmware page to the binary array. */
    firmware->index = index;
    memcpy(firmware->data, data, data_size);

    return ESP_OK;
}

#undef PROPERTY_TAG