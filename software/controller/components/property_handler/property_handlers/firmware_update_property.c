#include "firmware_update_property.h"
#include "esp_check.h"
#include "property_handler_common.h"

#include <string.h>

#define PROPERTY_TAG "FIRMWARE_UPDATE_PROPERTY_HANDLER"

//----------------------------------------------------------------------------------------------------------------------------------

esp_err_t firmware_update_property_set(module_t *module, uint16_t index, const uint8_t *data)
{
    assert(module != NULL);
    assert(data != NULL);

    /* Create new firmware update property. */
    firmware_update_property_t *new_firmware_update = firmware_update_new();
    ESP_RETURN_ON_FALSE(new_firmware_update != NULL, ESP_ERR_NO_MEM, PROPERTY_TAG, "Failed to allocate memory");

    /* Free the old data. */
    firmware_update_free(module->firmware_update);

    /* Update the module with the new firmware update. */
    module->firmware_update = new_firmware_update;

    /* Set the data. */
    module->firmware_update->index = index;
    uint16_t size = chain_comm_property_write_attributes_get(PROPERTY_FIRMWARE_UPDATE)->static_property_size - 2;
    memcpy(module->firmware_update->data, data, size);

    return ESP_OK;
}

//----------------------------------------------------------------------------------------------------------------------------------

/**
 * \brief Serialize the property into a byte array.
 *
 * \param[out] bin The serialized byte array.
 * \param[out] bin_size The size of the byte array.
 * \param[in] module The module containing the property.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static esp_err_t firmware_update_to_binary(uint8_t **bin, uint16_t *bin_size, const module_t *module)
{
    assert(bin != NULL);
    assert(bin_size != NULL);
    assert(module != NULL);

    /* Check if the firmware property has been initialized before. */
    ESP_RETURN_ON_FALSE(module->firmware_update != NULL, ESP_ERR_INVALID_ARG, PROPERTY_TAG,
                        "firmware update property is NULL");

    *bin_size = chain_comm_property_write_attributes_get(PROPERTY_FIRMWARE_UPDATE)->static_property_size;

    *bin = malloc(*bin_size);
    ESP_RETURN_ON_FALSE(*bin != NULL, ESP_ERR_NO_MEM, PROPERTY_TAG, "Failed to allocate memory");

    /* Copy the index and firmware page to the binary array. */
    (*bin)[0] = (module->firmware_update->index >> 8) & 0xFF;
    (*bin)[1] = module->firmware_update->index & 0xFF;
    memcpy((*bin) + 2, module->firmware_update->data, *bin_size);

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
static bool firmware_update_compare(const module_t *module_a, const module_t *module_b)
{
    assert(module_a != NULL);
    assert(module_b != NULL);

    const firmware_update_property_t *firmware_a = module_a->firmware_update;
    const firmware_update_property_t *firmware_b = module_b->firmware_update;

    /* Check if they have a value. */
    if (firmware_a == NULL || firmware_b == NULL) {
        return false;
    }

    /* Check if the pointers are the same. */
    if (firmware_a == firmware_b) {
        return true;
    }

    /* Check if the sizes are the same. */
    if (firmware_a->index != firmware_b->index) {
        return false;
    }

    uint16_t size = chain_comm_property_write_attributes_get(PROPERTY_FIRMWARE_UPDATE)->static_property_size - 2;

    /* Compare the character sets. */
    return memcmp(firmware_a->data, firmware_b->data, size) == 0;
}

/**
 * The firmware property handler.
 *
 * The firmware property is used to store the firmware data for the modules.
 */
const property_handler_t PROPERTY_HANDLER_FIRMWARE_UPDATE = {
    .id        = PROPERTY_FIRMWARE_UPDATE,
    .to_binary = firmware_update_to_binary,
    .compare   = firmware_update_compare,
};

#undef PROPERTY_TAG