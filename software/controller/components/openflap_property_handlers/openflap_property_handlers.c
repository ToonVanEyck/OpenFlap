#include "openflap_property_handlers.h"
#include "esp_check.h"

#include <string.h>

//======================================================================================================================
//                                                   MACROS a DEFINES
//======================================================================================================================

//======================================================================================================================
//                                                   FUNCTION PROTOTYPES
//======================================================================================================================

static module_t *bin_handler_args_validate(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size);
static module_t *json_handler_args_validate(void *userdata, uint16_t node_idx, void *data);
static bool compare_handler_args_validate(const void *userdata_a, const void *userdata_b);

//======================================================================================================================
//                                              PRIVATE PROPERTY HANDLERS
//======================================================================================================================

//======================================================================================================================
// FIRMWARE VERSION PROPERTY HANDLER
//======================================================================================================================

#define PROPERTY_TAG "FIRMWARE_VERSION_PROP"

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Deserialize a byte array into a property.
 *
 * \param[inout] userdata The display containing the module.
 * \param[in] node_idx The node index of the module in the display.
 * \param[in] buf The byte array to deserialize.
 * \param[in] size The size of the byte array.
 *
 * \return true if the conversion was successful, false otherwise.
 */
bool firmware_update_from_bin(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    module_t *module = bin_handler_args_validate(userdata, node_idx, buf, size);
    ESP_RETURN_ON_FALSE(module != NULL, false, PROPERTY_TAG, "Invalid arguments");

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
 * \param[in] userdata The module containing the property.
 * \param[in] node_idx The node index of the module. (Not used.)
 * \param[out] data The json object in which we will store the property.
 *
 * \return true if the conversion was successful, false otherwise.
 */
bool firmware_version_to_json(void *userdata, uint16_t node_idx, void *data)
{
    module_t *module = json_handler_args_validate(userdata, node_idx, data);
    ESP_RETURN_ON_FALSE(module != NULL, false, PROPERTY_TAG, "Invalid arguments");
    cJSON **json = (cJSON **)data;

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
bool firmware_version_compare(const void *userdata_a, const void *userdata_b)
{
    ESP_RETURN_ON_FALSE(compare_handler_args_validate(userdata_a, userdata_b), false, PROPERTY_TAG,
                        "Invalid arguments");

    const firmware_version_property_t *version_a = ((const module_t *)userdata_a)->firmware_version;
    const firmware_version_property_t *version_b = ((const module_t *)userdata_b)->firmware_version;

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

//======================================================================================================================
// FIRMWARE UPDATE PROPERTY HANDLER
//======================================================================================================================

#define PROPERTY_TAG "FIRMWARE_UPDATE_PROP"

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Serialize the property into a byte array.
 *
 * \param[inout] userdata The display containing the module.
 * \param[in] node_idx The node index of the module in the display.
 * \param[out] buf The byte array to serialize.
 * \param[out] size The size of the byte array after serialization.
 *
 * \return true if the conversion was successful, false otherwise.
 */
bool firmware_update_to_bin(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    module_t *module = bin_handler_args_validate(userdata, node_idx, buf, size);
    ESP_RETURN_ON_FALSE(module != NULL, false, PROPERTY_TAG, "Invalid arguments");

    *size = OF_FIRMWARE_UPDATE_PAGE_SIZE + 2; /* 2 bytes for index */

    /* Copy the index and firmware page to the binary array. */
    buf[0] = (module->firmware_update->index >> 8) & 0xFF;
    buf[1] = module->firmware_update->index & 0xFF;
    memcpy(buf + 2, module->firmware_update->data, *size - 2);

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
bool firmware_update_compare(const void *userdata_a, const void *userdata_b)
{
    ESP_RETURN_ON_FALSE(compare_handler_args_validate(userdata_a, userdata_b), false, PROPERTY_TAG,
                        "Invalid arguments");

    const firmware_update_property_t *firmware_a = ((const module_t *)userdata_a)->firmware_update;
    const firmware_update_property_t *firmware_b = ((const module_t *)userdata_b)->firmware_update;

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

    /* Compare the character sets. */
    return memcmp(firmware_a->data, firmware_b->data, OF_FIRMWARE_UPDATE_PAGE_SIZE + 2) == 0;
}

//----------------------------------------------------------------------------------------------------------------------

#undef PROPERTY_TAG

//======================================================================================================================
// COMMAND PROPERTY HANDLER
//======================================================================================================================

#define PROPERTY_TAG "COMMAND_PROP"

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Serialize the property into a byte array.
 *
 * \param[inout] userdata The display containing the module.
 * \param[in] node_idx The node index of the module in the display.
 * \param[out] buf The byte array to serialize.
 * \param[out] size The size of the byte array after serialization.
 *
 * \return true if the conversion was successful, false otherwise.
 */
bool command_to_bin(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    module_t *module = bin_handler_args_validate(userdata, node_idx, buf, size);
    ESP_RETURN_ON_FALSE(module != NULL, false, PROPERTY_TAG, "Invalid arguments");

    const command_property_cmd_t *command = &module->command;

    *size = 1;

    buf[0] = *command;

    return true;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Populate the property from a json object.
 *
 * \param[inout] userdata The module containing the property.
 * \param[in] node_idx The node index of the module. (Not used.)
 * \param[in] data The json object in which we will store the property.
 *
 * \return true if the conversion was successful, false otherwise.
 */
bool command_from_json(void *userdata, uint16_t node_idx, void *data)
{
    module_t *module = json_handler_args_validate(userdata, node_idx, data);
    ESP_RETURN_ON_FALSE(module != NULL, false, PROPERTY_TAG, "Invalid arguments");
    cJSON *json = (cJSON *)data;

    command_property_cmd_t *command = &module->command;

    ESP_RETURN_ON_FALSE(cJSON_IsString(json), false, PROPERTY_TAG, "Expected a string");

    *command = of_cmd_id_by_name(json->valuestring);
    if (*command != CMD_UNDEFINED) {
        return true;
    }

    ESP_LOGE(PROPERTY_TAG, "Command \"%s\" is not supported.", json->valuestring);
    return false;
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
bool command_compare(const void *userdata_a, const void *userdata_b)
{
    ESP_RETURN_ON_FALSE(compare_handler_args_validate(userdata_a, userdata_b), false, PROPERTY_TAG,
                        "Invalid arguments");

    const command_property_cmd_t command_a = ((const module_t *)userdata_a)->command;
    const command_property_cmd_t command_b = ((const module_t *)userdata_b)->command;

    /* Compare the commands. */
    return command_a == command_b;
}

//----------------------------------------------------------------------------------------------------------------------

#undef PROPERTY_TAG

//----------------------------------------------------------------------------------------------------------------------

void of_property_handlers_init(void)
{
    cc_prop_list[OF_CC_PROP_FIRMWARE_VERSION].handler.set     = firmware_update_from_bin;
    cc_prop_list[OF_CC_PROP_FIRMWARE_VERSION].handler.get     = NULL; /* Not implemented : Read Only */
    cc_prop_list[OF_CC_PROP_FIRMWARE_VERSION].handler.get_alt = firmware_version_to_json;
    cc_prop_list[OF_CC_PROP_FIRMWARE_VERSION].handler.set_alt = NULL; /* Not implemented : Read Only */
    cc_prop_list[OF_CC_PROP_FIRMWARE_VERSION].handler.compare = firmware_version_compare;

    cc_prop_list[OF_CC_PROP_FIRMWARE_UPDATE].handler.set     = NULL; /* Not implemented : Write Only */
    cc_prop_list[OF_CC_PROP_FIRMWARE_UPDATE].handler.get     = firmware_update_to_bin;
    cc_prop_list[OF_CC_PROP_FIRMWARE_UPDATE].handler.get_alt = NULL; /* Not implemented : Write Only */
    cc_prop_list[OF_CC_PROP_FIRMWARE_UPDATE].handler.set_alt = NULL; /* Special File Based write elsewhere. */
    cc_prop_list[OF_CC_PROP_FIRMWARE_UPDATE].handler.compare = firmware_update_compare;

    cc_prop_list[OF_CC_PROP_COMMAND].handler.set     = NULL; /* Not implemented : Write Only */
    cc_prop_list[OF_CC_PROP_COMMAND].handler.get     = command_to_bin;
    cc_prop_list[OF_CC_PROP_COMMAND].handler.get_alt = NULL; /* Not implemented : Write Only */
    cc_prop_list[OF_CC_PROP_COMMAND].handler.set_alt = command_from_json;
    cc_prop_list[OF_CC_PROP_COMMAND].handler.compare = command_compare;
}
//======================================================================================================================
//                                                         PRIVATE FUNCTIONS
//======================================================================================================================

/**
 * \brief Validate and extract the module from the binary handler arguments.
 *
 * \param[in] userdata The user data containing the display.
 * \param[in] node_idx The node index of the module.
 * \param[in] buf The binary buffer.
 * \param[in] size The size of the binary buffer.
 *
 * \return The module if valid, NULL otherwise.
 */
static module_t *bin_handler_args_validate(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    if (!userdata || !buf || !size) {
        return NULL;
    }
    return display_module_get((of_display_t *)userdata, node_idx);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Validate and extract the module from the json handler arguments.
 *
 * \param[in] userdata The user data containing the display.
 * \param[in] node_idx The node index of the module.
 * \param[in] data The json data.
 *
 * \return The module if valid, NULL otherwise.
 */
static module_t *json_handler_args_validate(void *userdata, uint16_t node_idx, void *data)
{
    (void)node_idx;
    if (!userdata || !data) {
        return NULL;
    }
    return (module_t *)userdata;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Validate the compare handler arguments.
 *
 * \param[in] userdata_a The user data of the first module.
 * \param[in] userdata_b The user data of the second module.
 *
 * \return true if both user data are valid, false otherwise.
 */
static bool compare_handler_args_validate(const void *userdata_a, const void *userdata_b)
{
    return userdata_a && userdata_b;
}