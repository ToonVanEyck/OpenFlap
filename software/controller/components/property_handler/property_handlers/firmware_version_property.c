#include "esp_check.h"
#include "property_handler_common.h"

#include <string.h>

#define PROPERTY_TAG "FIRMWARE_VERSION_PROPERTY_HANDLER"

//----------------------------------------------------------------------------------------------------------------------------------

/**
 * \brief Convert the property into it's json representation.
 *
 * \param[out] json The json object in which we will store the property.
 * \param[in] module The module containing the property.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static esp_err_t firmware_version_to_json(cJSON **json, const module_t *module)
{
    assert(json != NULL);
    assert(module != NULL);

    *json = cJSON_CreateString(module->firmware_version->str);
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
static esp_err_t firmware_version_from_binary(module_t *module, const uint8_t *bin, uint16_t bin_size)
{
    assert(module != NULL);
    assert(bin != NULL);

    /* Create new firmware version property. */
    firmware_version_property_t *new_firmware_version = firmware_version_new(bin_size);
    ESP_RETURN_ON_FALSE(new_firmware_version != NULL, ESP_ERR_NO_MEM, PROPERTY_TAG, "Failed to allocate memory");

    /* Free the old data. */
    firmware_version_free(module->firmware_version);

    /* Update the module with the new firmware version. */
    module->firmware_version = new_firmware_version;

    /* Copy the binary array to the firmware version. */
    memcpy(module->firmware_version->str, bin, bin_size);

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
static bool firmware_version_compare(const module_t *module_a, const module_t *module_b)
{
    assert(module_a != NULL);
    assert(module_b != NULL);

    const firmware_version_property_t *version_a = module_a->firmware_version;
    const firmware_version_property_t *version_b = module_b->firmware_version;

    /* Check if they have a value. */
    if (version_a == NULL || version_b == NULL) {
        return false;
    }

    /* Check if the pointers are the same. */
    if (version_a == version_b) {
        return true;
    }

    /* Compare the strings. */
    return strcmp(version_a->str, version_b->str) == 0;
}

/**
 * The firmware property handler.
 *
 * The firmware property is used to store the firmware data for the modules.
 */
const property_handler_t PROPERTY_HANDLER_FIRMWARE_VERSION = {
    .id          = PROPERTY_FIRMWARE_VERSION,
    .to_json     = firmware_version_to_json,
    .from_binary = firmware_version_from_binary,
    .compare     = firmware_version_compare,
};

#undef PROPERTY_TAG