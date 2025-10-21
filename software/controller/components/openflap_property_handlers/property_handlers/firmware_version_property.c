#include "esp_check.h"
#include "openflap_property_handler.h"

#include <string.h>

#define PROPERTY_TAG "FIRMWARE_VERSION_PROPERTY_HANDLER"

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Deserialize a byte array into a property.
 *
 * \param[out] module The module containing the property.
 * \param[in] bin The byte array to deserialize.
 * \param[in] bin_size The size of the byte array.
 *
 * \return true if the conversion was successful, false otherwise.
 */
bool prop_handler_from_bin_firmware_update(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    assert(userdata != NULL);
    assert(buf != NULL);
    assert(size != NULL);

    module_t *module = display_module_get((of_display_t *)userdata, node_idx);
    ESP_RETURN_ON_FALSE(module != NULL, false, PROPERTY_TAG, "Module not found");

    /* Create new firmware version property. */
    firmware_version_property_t *new_firmware_version = firmware_version_new(*size - 4);
    ESP_RETURN_ON_FALSE(new_firmware_version != NULL, false, PROPERTY_TAG, "Failed to allocate memory");

    /* Free the old data. */
    firmware_version_free(module->firmware_version);

    /* Update the module with the new firmware version. */
    module->firmware_version = new_firmware_version;

    /* Copy the binary array to the firmware version. */
    memcpy(module->firmware_version->str, buf, *size - 4);

    /* Read the CRC from the last 4 bytes of the binary array. */
    module->firmware_crc = 0;
    for (size_t i = 0; i < 4; i++) {
        module->firmware_crc |= (uint32_t)buf[*size - 4 + i] << (i * 8);
    }

    return true;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Convert the property into it's json representation.
 *
 * \param[out] json The json object in which we will store the property.
 * \param[in] module The module containing the property.
 *
 * \return true if the conversion was successful, false otherwise.
 */
bool prop_handler_to_json_firmware_version(void *userdata, uint16_t node_idx, void *data)
{
    assert(userdata != NULL);
    (void)node_idx; /* Not used because we are getting a module pointer directly. */
    assert(data != NULL);

    module_t *module = (module_t *)userdata;
    cJSON **json     = (cJSON **)data;

    ESP_RETURN_ON_FALSE(cJSON_AddStringToObject(*json, "description", module->firmware_version->str) != NULL, false,
                        PROPERTY_TAG, "Failed to create JSON string");

    char crc_str[9] = {0};
    snprintf(crc_str, sizeof(crc_str), "%08lX", module->firmware_crc);
    ESP_RETURN_ON_FALSE(cJSON_AddStringToObject(*json, "crc", crc_str) != NULL, false, PROPERTY_TAG,
                        "Failed to create JSON string");

    return true;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Compare the properties of two modules.
 *
 * \param[in] module_a The first module to compare.
 * \param[in] module_b The second module to compare.
 *
 * \return true if the properties are the same, false otherwise.
 */
bool prop_handler_compare_firmware_version(const void *userdata_a, const void *userdata_b)
{
    assert(userdata_a != NULL);
    assert(userdata_b != NULL);

    const module_t *module_a = (const module_t *)userdata_a;
    const module_t *module_b = (const module_t *)userdata_b;

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

//----------------------------------------------------------------------------------------------------------------------

#undef PROPERTY_TAG