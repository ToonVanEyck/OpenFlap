#include "openflap_property_handlers.h"
#include "esp_check.h"

#include <string.h>

//======================================================================================================================
//                                                   MACROS a DEFINES
//======================================================================================================================

#define TAG "PROP_HANDLERS"

//======================================================================================================================
//                                                   FUNCTION PROTOTYPES
//======================================================================================================================

static module_t *bin_handler_args_validate(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size);
static module_t *json_handler_args_validate(void *userdata, uint16_t node_idx, void *data);
static bool compare_handler_args_validate(const void *userdata_a, const void *userdata_b);
static esp_err_t json_to_color(color_t *color, const cJSON *json);

//======================================================================================================================
//                                              PRIVATE PROPERTY HANDLERS
//======================================================================================================================

//======================================================================================================================
// FIRMWARE VERSION PROPERTY HANDLER
//======================================================================================================================

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
bool firmware_version_from_bin(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    module_t *module = bin_handler_args_validate(userdata, node_idx, buf, size);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");

    /* Create new firmware version property. */
    firmware_version_property_t *new_firmware_version = firmware_version_new(*size - 4);
    ESP_RETURN_ON_FALSE(new_firmware_version != NULL, false, TAG, "Failed to allocate memory");

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
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");
    cJSON **json = (cJSON **)data;

    ESP_RETURN_ON_FALSE(cJSON_AddStringToObject(*json, "description", module->firmware_version->str) != NULL, false,
                        TAG, "Failed to create JSON string");

    char crc_str[9] = {0};
    snprintf(crc_str, sizeof(crc_str), "%08lX", module->firmware_crc);
    ESP_RETURN_ON_FALSE(cJSON_AddStringToObject(*json, "crc", crc_str) != NULL, false, TAG,
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
    ESP_RETURN_ON_FALSE(compare_handler_args_validate(userdata_a, userdata_b), false, TAG, "Invalid arguments");

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

//======================================================================================================================
// FIRMWARE UPDATE PROPERTY HANDLER
//======================================================================================================================

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
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");

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
    ESP_RETURN_ON_FALSE(compare_handler_args_validate(userdata_a, userdata_b), false, TAG, "Invalid arguments");

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
    return memcmp(firmware_a->data, firmware_b->data, OF_FIRMWARE_UPDATE_PAGE_SIZE) == 0;
}

//======================================================================================================================
// COMMAND PROPERTY HANDLER
//======================================================================================================================

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
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");

    *size  = 1;
    buf[0] = module->command;

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
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");
    cJSON *json = (cJSON *)data;

    ESP_RETURN_ON_FALSE(cJSON_IsString(json), false, TAG, "Expected a string");

    module->command = of_cmd_id_by_name(json->valuestring);
    if (module->command != CMD_UNDEFINED) {
        return true;
    }

    ESP_LOGE(TAG, "Command \"%s\" is not supported.", json->valuestring);
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
    ESP_RETURN_ON_FALSE(compare_handler_args_validate(userdata_a, userdata_b), false, TAG, "Invalid arguments");

    const command_property_cmd_t command_a = ((const module_t *)userdata_a)->command;
    const command_property_cmd_t command_b = ((const module_t *)userdata_b)->command;

    /* Compare the commands. */
    return command_a == command_b;
}

//======================================================================================================================
// MODULE INFO PROPERTY HANDLER
//======================================================================================================================

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
bool module_info_from_bin(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    module_t *module = bin_handler_args_validate(userdata, node_idx, buf, size);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");

    module->column_end = buf[0];

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
bool module_info_to_json(void *userdata, uint16_t node_idx, void *data)
{
    module_t *module = json_handler_args_validate(userdata, node_idx, data);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");
    cJSON **json = (cJSON **)data;

    ESP_RETURN_ON_FALSE(cJSON_AddBoolToObject(*json, "column_end", module->column_end), false, TAG,
                        "Failed to create JSON boolean");

    return true;
}

//======================================================================================================================
// CHARACTER SET PROPERTY HANDLER
//======================================================================================================================

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
bool character_set_from_bin(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    module_t *module = bin_handler_args_validate(userdata, node_idx, buf, size);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");

    ESP_RETURN_ON_FALSE(*size % 4 == 0, false, TAG, "Invalid binary size, expected multiple of 4.");

    /* Allocate memory for the character set. */
    character_set_property_t *new_character_set = character_set_new(*size / 4);
    ESP_RETURN_ON_FALSE(new_character_set != NULL, false, TAG, "Memory allocation failed");

    /* Free old character set. */
    character_set_free(module->character_set);

    /* Update the module with the new character set. */
    module->character_set = new_character_set;

    /* Copy the binary array to the character set. */
    memcpy(module->character_set->data, buf, module->character_set->size * 4);

    return true;
}

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
bool character_set_to_bin(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    module_t *module = bin_handler_args_validate(userdata, node_idx, buf, size);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");

    /* Check if the character set has been initialized before. */
    ESP_RETURN_ON_FALSE(module->character_set->size > 0, false, TAG, "Character set size is 0");
    ESP_RETURN_ON_FALSE(module->character_set->data != NULL, false, TAG, "Character set is NULL");

    *size = module->character_set->size * 4;

    /* Copy the character set to the binary array. */
    memcpy(buf, module->character_set->data, *size);

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
bool character_set_from_json(void *userdata, uint16_t node_idx, void *data)
{
    module_t *module = json_handler_args_validate(userdata, node_idx, data);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");
    cJSON *json = (cJSON *)data;

    /* Validate json. */
    ESP_RETURN_ON_FALSE(cJSON_IsArray(json), false, TAG, "Expected an array of strings");

    cJSON *character_set_entry = NULL;
    cJSON_ArrayForEach(character_set_entry, json)
    {
        ESP_RETURN_ON_FALSE(cJSON_IsString(character_set_entry), false, TAG, "Expected an array of strings but got %s",
                            character_set_entry->valuestring);
    }

    /* Get the character set size and allocate memory. */
    ESP_RETURN_ON_FALSE(module->character_set->size == cJSON_GetArraySize(json), false, TAG,
                        "Expected a character set of size %d", module->character_set->size);

    /* Allocate memory for the new character set. */
    character_set_property_t *new_character_set = character_set_new(module->character_set->size);
    ESP_RETURN_ON_FALSE(new_character_set != NULL, false, TAG, "Memory allocation failed");

    /* Free the old character set. */
    character_set_free(module->character_set);

    /* Update the module with the new character set. */
    module->character_set = new_character_set;

    /* Copy the character set from the json object. */
    for (int i = 0; i < module->character_set->size; i++) {
        /* Get new entry from json. */
        character_set_entry = cJSON_GetArrayItem(json, i);
        /* Copy new entry. */
        strncpy((char *)(&module->character_set->data[i * 4]), character_set_entry->valuestring, 4);
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
bool character_set_to_json(void *userdata, uint16_t node_idx, void *data)
{
    module_t *module = json_handler_args_validate(userdata, node_idx, data);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");
    cJSON **json = (cJSON **)data;

    ESP_RETURN_ON_FALSE(module->character_set->size > 0, false, TAG, "Character set size is 0");

    *json = cJSON_CreateArray();
    ESP_RETURN_ON_FALSE(*json != NULL, false, TAG, "Memory allocation failed");

    for (int i = 0; i < module->character_set->size; i++) {
        cJSON *character_entry = cJSON_CreateString((char *)(&module->character_set->data[i * 4]));
        ESP_RETURN_ON_FALSE(character_entry != NULL, false, TAG, "Memory allocation failed");
        cJSON_AddItemToArray(*json, character_entry);
    }

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
bool character_set_compare(const void *userdata_a, const void *userdata_b)
{
    ESP_RETURN_ON_FALSE(compare_handler_args_validate(userdata_a, userdata_b), false, TAG, "Invalid arguments");

    const character_set_property_t *character_set_a = ((const module_t *)userdata_a)->character_set;
    const character_set_property_t *character_set_b = ((const module_t *)userdata_b)->character_set;

    /* Check if they have a value. */
    if (character_set_a == NULL || character_set_b == NULL) {
        return false;
    }

    /* Check if the pointers are the same. */
    if (character_set_a == character_set_b) {
        return true;
    }

    /* Check if the sizes are the same. */
    if (character_set_a->size != character_set_b->size) {
        return false;
    }

    return memcmp(character_set_a->data, character_set_b->data, character_set_a->size * 4) == 0;
}

//======================================================================================================================
// CHARACTER PROPERTY HANDLER
//======================================================================================================================

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
bool character_from_bin(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    module_t *module = bin_handler_args_validate(userdata, node_idx, buf, size);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");

    module->character_index = buf[0];

    return true;
}

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
bool character_to_bin(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    module_t *module = bin_handler_args_validate(userdata, node_idx, buf, size);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");

    *size  = 1; /* Only one byte, the character index. */
    buf[0] = module->character_index;

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
bool character_from_json(void *userdata, uint16_t node_idx, void *data)
{
    module_t *module = json_handler_args_validate(userdata, node_idx, data);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");
    cJSON *json = (cJSON *)data;

    uint8_t *character_index = &module->character_index;

    ESP_RETURN_ON_FALSE(cJSON_IsString(json), false, TAG, "Expected a character");

    ESP_RETURN_ON_FALSE(module_character_set_index_of_character(module, character_index, json->valuestring) == ESP_OK,
                        false, TAG, "Character %s not in character set", json->valuestring);

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
bool character_to_json(void *userdata, uint16_t node_idx, void *data)
{
    module_t *module = json_handler_args_validate(userdata, node_idx, data);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");
    cJSON **json = (cJSON **)data;

    const uint8_t *character_index = &module->character_index;

    *json = cJSON_CreateString((const char *)&module->character_set->data[4 * (*character_index)]);
    ESP_RETURN_ON_FALSE(*json != NULL, false, TAG, "Failed to create JSON string");

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
bool character_compare(const void *userdata_a, const void *userdata_b)
{
    ESP_RETURN_ON_FALSE(compare_handler_args_validate(userdata_a, userdata_b), false, TAG, "Invalid arguments");

    const uint8_t character_index_a = ((const module_t *)userdata_a)->character_index;
    const uint8_t character_index_b = ((const module_t *)userdata_b)->character_index;

    return character_index_a == character_index_b;
}

//======================================================================================================================
// OFFSET PROPERTY HANDLER
//======================================================================================================================

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
bool offset_from_bin(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    module_t *module = bin_handler_args_validate(userdata, node_idx, buf, size);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");

    module->offset = buf[0];

    return true;
}

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
bool offset_to_bin(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    module_t *module = bin_handler_args_validate(userdata, node_idx, buf, size);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");

    *size  = 1;
    buf[0] = module->offset;

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
bool offset_from_json(void *userdata, uint16_t node_idx, void *data)
{
    module_t *module = json_handler_args_validate(userdata, node_idx, data);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");
    cJSON *json = (cJSON *)data;

    ESP_RETURN_ON_FALSE(cJSON_IsNumber(json), false, TAG, "Expected a number for offset");

    ESP_RETURN_ON_FALSE(json->valueint >= 0 && json->valueint <= 255, false, TAG, "Value out of range");

    module->offset = json->valueint;

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
bool offset_to_json(void *userdata, uint16_t node_idx, void *data)
{
    module_t *module = json_handler_args_validate(userdata, node_idx, data);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");
    cJSON **json = (cJSON **)data;

    *json = cJSON_CreateNumber(module->offset);
    ESP_RETURN_ON_FALSE(*json != NULL, false, TAG, "Failed to create JSON number");

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
bool offset_compare(const void *userdata_a, const void *userdata_b)
{
    ESP_RETURN_ON_FALSE(compare_handler_args_validate(userdata_a, userdata_b), false, TAG, "Invalid arguments");

    const uint8_t offset_a = ((const module_t *)userdata_a)->offset;
    const uint8_t offset_b = ((const module_t *)userdata_b)->offset;

    return offset_a == offset_b;
}

//======================================================================================================================
// COLOR PROPERTY HANDLER
//======================================================================================================================

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
bool color_from_bin(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    module_t *module = bin_handler_args_validate(userdata, node_idx, buf, size);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");

    module->color.foreground.red   = buf[0];
    module->color.foreground.green = buf[1];
    module->color.foreground.blue  = buf[2];
    module->color.background.red   = buf[3];
    module->color.background.green = buf[4];
    module->color.background.blue  = buf[5];

    return true;
}

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
bool color_to_bin(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    module_t *module = bin_handler_args_validate(userdata, node_idx, buf, size);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");

    *size  = 6;
    buf[0] = module->color.foreground.red;
    buf[1] = module->color.foreground.green;
    buf[2] = module->color.foreground.blue;
    buf[3] = module->color.background.red;
    buf[4] = module->color.background.green;
    buf[5] = module->color.background.blue;
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
bool color_from_json(void *userdata, uint16_t node_idx, void *data)
{
    module_t *module = json_handler_args_validate(userdata, node_idx, data);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");
    cJSON *json = (cJSON *)data;

    cJSON *foreground_json = cJSON_GetObjectItem(json, "foreground");
    cJSON *background_json = cJSON_GetObjectItem(json, "background");

    color_t foreground = {0};
    color_t background = {0};

    ESP_RETURN_ON_FALSE(json_to_color(&foreground, foreground_json) == ESP_OK, false, TAG,
                        "Failed to convert foreground color");
    ESP_RETURN_ON_FALSE(json_to_color(&background, background_json) == ESP_OK, false, TAG,
                        "Failed to convert background color");

    module->color.foreground = foreground;
    module->color.background = background;

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
bool color_to_json(void *userdata, uint16_t node_idx, void *data)
{
    module_t *module = json_handler_args_validate(userdata, node_idx, data);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");
    cJSON **json = (cJSON **)data;

    char fg_color_str[8] = {0};
    char bg_color_str[8] = {0};

    snprintf(fg_color_str, sizeof(fg_color_str), "#%06X",
             module->color.foreground.red << 16 | module->color.foreground.green << 8 | module->color.foreground.blue);
    snprintf(bg_color_str, sizeof(bg_color_str), "#%06X",
             module->color.background.red << 16 | module->color.background.green << 8 | module->color.background.blue);

    ESP_RETURN_ON_FALSE(cJSON_AddStringToObject(*json, "foreground", fg_color_str), false, TAG,
                        "Failed to create JSON string for foreground color");
    ESP_RETURN_ON_FALSE(cJSON_AddStringToObject(*json, "background", bg_color_str), false, TAG,
                        "Failed to create JSON string for background color");

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
bool color_compare(const void *userdata_a, const void *userdata_b)
{
    ESP_RETURN_ON_FALSE(compare_handler_args_validate(userdata_a, userdata_b), false, TAG, "Invalid arguments");

    const color_property_t *color_a = &((const module_t *)userdata_a)->color;
    const color_property_t *color_b = &((const module_t *)userdata_b)->color;

    return memcmp(color_a, color_b, sizeof(color_property_t)) == 0;
}

//======================================================================================================================
// MOTION PROPERTY HANDLER
//======================================================================================================================

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
bool motion_from_bin(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    module_t *module = bin_handler_args_validate(userdata, node_idx, buf, size);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");

    module->motion.speed_min           = buf[0];
    module->motion.speed_max           = buf[1];
    module->motion.distance_ramp_start = buf[2];
    module->motion.distance_ramp_stop  = buf[3];

    return true;
}

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
bool motion_to_bin(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    module_t *module = bin_handler_args_validate(userdata, node_idx, buf, size);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");

    *size  = 4;
    buf[0] = module->motion.speed_min;
    buf[1] = module->motion.speed_max;
    buf[2] = module->motion.distance_ramp_start;
    buf[3] = module->motion.distance_ramp_stop;
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
bool motion_from_json(void *userdata, uint16_t node_idx, void *data)
{
    module_t *module = json_handler_args_validate(userdata, node_idx, data);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");
    cJSON *json = (cJSON *)data;

    cJSON *speed_min_json  = cJSON_GetObjectItem(json, "speed_min");
    cJSON *speed_max_json  = cJSON_GetObjectItem(json, "speed_max");
    cJSON *ramp_start_json = cJSON_GetObjectItem(json, "ramp_start");
    cJSON *ramp_stop_json  = cJSON_GetObjectItem(json, "ramp_stop");

    ESP_RETURN_ON_FALSE(cJSON_IsNumber(speed_min_json), false, TAG, "Expected a number for speed_min");
    ESP_RETURN_ON_FALSE(cJSON_IsNumber(speed_max_json), false, TAG, "Expected a number for speed_max");
    ESP_RETURN_ON_FALSE(cJSON_IsNumber(ramp_start_json), false, TAG, "Expected a number for ramp_start");
    ESP_RETURN_ON_FALSE(cJSON_IsNumber(ramp_stop_json), false, TAG, "Expected a number for ramp_stop");

    ESP_RETURN_ON_FALSE(speed_min_json->valueint >= 0 && speed_min_json->valueint <= 255, false, TAG,
                        "Value speed_min out of range");
    ESP_RETURN_ON_FALSE(speed_max_json->valueint >= 0 && speed_max_json->valueint <= 255, false, TAG,
                        "Value speed_max out of range");
    ESP_RETURN_ON_FALSE(ramp_start_json->valueint >= 0 && ramp_start_json->valueint <= 255, false, TAG,
                        "Value ramp_start out of range");
    ESP_RETURN_ON_FALSE(ramp_stop_json->valueint >= 0 && ramp_stop_json->valueint <= 255, false, TAG,
                        "Value ramp_stop out of range");

    module->motion.speed_min           = speed_min_json->valueint;
    module->motion.speed_max           = speed_max_json->valueint;
    module->motion.distance_ramp_start = ramp_start_json->valueint;
    module->motion.distance_ramp_stop  = ramp_stop_json->valueint;

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
bool motion_to_json(void *userdata, uint16_t node_idx, void *data)
{
    module_t *module = json_handler_args_validate(userdata, node_idx, data);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");
    cJSON **json = (cJSON **)data;

    ESP_RETURN_ON_FALSE(cJSON_AddNumberToObject(*json, "speed_min", module->motion.speed_min), false, TAG,
                        "Failed to create JSON number for speed_min");
    ESP_RETURN_ON_FALSE(cJSON_AddNumberToObject(*json, "speed_max", module->motion.speed_max), false, TAG,
                        "Failed to create JSON number for speed_max");
    ESP_RETURN_ON_FALSE(cJSON_AddNumberToObject(*json, "ramp_start", module->motion.distance_ramp_start), false, TAG,
                        "Failed to create JSON number for ramp_start");
    ESP_RETURN_ON_FALSE(cJSON_AddNumberToObject(*json, "ramp_stop", module->motion.distance_ramp_stop), false, TAG,
                        "Failed to create JSON number for ramp_stop");

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
bool motion_compare(const void *userdata_a, const void *userdata_b)
{
    ESP_RETURN_ON_FALSE(compare_handler_args_validate(userdata_a, userdata_b), false, TAG, "Invalid arguments");

    const motion_property_t *motion_a = &((const module_t *)userdata_a)->motion;
    const motion_property_t *motion_b = &((const module_t *)userdata_b)->motion;

    return memcmp(motion_a, motion_b, sizeof(motion_property_t)) == 0;
}

//======================================================================================================================
// MIN ROTATION PROPERTY HANDLER
//======================================================================================================================

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
bool min_rotation_from_bin(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    module_t *module = bin_handler_args_validate(userdata, node_idx, buf, size);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");

    module->minimum_rotation = buf[0];

    return true;
}

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
bool min_rotation_to_bin(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    module_t *module = bin_handler_args_validate(userdata, node_idx, buf, size);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");

    *size  = 1;
    buf[0] = module->minimum_rotation;
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
bool min_rotation_from_json(void *userdata, uint16_t node_idx, void *data)
{
    module_t *module = json_handler_args_validate(userdata, node_idx, data);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");
    cJSON *json = (cJSON *)data;

    ESP_RETURN_ON_FALSE(cJSON_IsNumber(json), false, TAG, "Expected a number for minimum_rotation");

    ESP_RETURN_ON_FALSE(json->valueint >= 0 && json->valueint <= 255, false, TAG, "Value out of range");

    module->minimum_rotation = json->valueint;

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
bool min_rotation_to_json(void *userdata, uint16_t node_idx, void *data)
{
    module_t *module = json_handler_args_validate(userdata, node_idx, data);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");
    cJSON **json = (cJSON **)data;

    *json = cJSON_CreateNumber(module->minimum_rotation);

    ESP_RETURN_ON_FALSE(*json != NULL, false, TAG, "Failed to create JSON number for minimum rotation");

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
bool min_rotation_compare(const void *userdata_a, const void *userdata_b)
{
    ESP_RETURN_ON_FALSE(compare_handler_args_validate(userdata_a, userdata_b), false, TAG, "Invalid arguments");

    const uint8_t min_rotation_a = ((const module_t *)userdata_a)->minimum_rotation;
    const uint8_t min_rotation_b = ((const module_t *)userdata_b)->minimum_rotation;

    return min_rotation_a == min_rotation_b;
}

//======================================================================================================================
// IR THRESHOLD PROPERTY HANDLER
//======================================================================================================================

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
bool ir_threshold_from_bin(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    module_t *module = bin_handler_args_validate(userdata, node_idx, buf, size);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");

    module->ir_threshold.lower = (uint16_t)buf[0] << 8 | (uint16_t)buf[1];
    module->ir_threshold.upper = (uint16_t)buf[2] << 8 | (uint16_t)buf[3];

    return true;
}

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
bool ir_threshold_to_bin(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    module_t *module = bin_handler_args_validate(userdata, node_idx, buf, size);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");

    *size  = 4;
    buf[0] = module->ir_threshold.lower >> 8;
    buf[1] = module->ir_threshold.lower & 0xFF;
    buf[2] = module->ir_threshold.upper >> 8;
    buf[3] = module->ir_threshold.upper & 0xFF;
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
bool ir_threshold_from_json(void *userdata, uint16_t node_idx, void *data)
{
    module_t *module = json_handler_args_validate(userdata, node_idx, data);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");
    cJSON *json = (cJSON *)data;

    cJSON *lower_json = cJSON_GetObjectItem(json, "lower");
    cJSON *upper_json = cJSON_GetObjectItem(json, "upper");

    ESP_RETURN_ON_FALSE(cJSON_IsNumber(lower_json), false, TAG, "Expected a number for lower");
    ESP_RETURN_ON_FALSE(cJSON_IsNumber(upper_json), false, TAG, "Expected a number for upper");

    ESP_RETURN_ON_FALSE(lower_json->valueint >= 0 && lower_json->valueint <= 1024, false, TAG,
                        "Value lower_json out of range");
    ESP_RETURN_ON_FALSE(upper_json->valueint >= 0 && upper_json->valueint <= 1024, false, TAG,
                        "Value upper_json out of range");
    ESP_RETURN_ON_FALSE(lower_json->valueint < upper_json->valueint, false, TAG,
                        "Value lower_json must be less than upper_json");

    module->ir_threshold.lower = lower_json->valueint;
    module->ir_threshold.upper = upper_json->valueint;

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
bool ir_threshold_to_json(void *userdata, uint16_t node_idx, void *data)
{
    module_t *module = json_handler_args_validate(userdata, node_idx, data);
    ESP_RETURN_ON_FALSE(module != NULL, false, TAG, "Invalid arguments");
    cJSON **json = (cJSON **)data;

    ESP_RETURN_ON_FALSE(cJSON_AddNumberToObject(*json, "lower", module->ir_threshold.lower), false, TAG,
                        "Failed to create JSON number for lower");

    ESP_RETURN_ON_FALSE(cJSON_AddNumberToObject(*json, "upper", module->ir_threshold.upper), false, TAG,
                        "Failed to create JSON number for upper");

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
bool ir_threshold_compare(const void *userdata_a, const void *userdata_b)
{
    ESP_RETURN_ON_FALSE(compare_handler_args_validate(userdata_a, userdata_b), false, TAG, "Invalid arguments");

    const ir_threshold_property_t *ir_threshold_a = &((const module_t *)userdata_a)->ir_threshold;
    const ir_threshold_property_t *ir_threshold_b = &((const module_t *)userdata_b)->ir_threshold;

    return memcmp(ir_threshold_a, ir_threshold_b, sizeof(ir_threshold_property_t)) == 0;
}

//----------------------------------------------------------------------------------------------------------------------

void of_property_handlers_init(void)
{
    mdl_prop_list[OF_MDL_PROP_FIRMWARE_VERSION].handler.set     = firmware_version_from_bin;
    mdl_prop_list[OF_MDL_PROP_FIRMWARE_VERSION].handler.get     = NULL; /* Not implemented : Read Only */
    mdl_prop_list[OF_MDL_PROP_FIRMWARE_VERSION].handler.get_alt = firmware_version_to_json;
    mdl_prop_list[OF_MDL_PROP_FIRMWARE_VERSION].handler.set_alt = NULL; /* Not implemented : Read Only */
    mdl_prop_list[OF_MDL_PROP_FIRMWARE_VERSION].handler.compare = firmware_version_compare;

    mdl_prop_list[OF_MDL_PROP_FIRMWARE_UPDATE].handler.set     = NULL; /* Not implemented : Write Only */
    mdl_prop_list[OF_MDL_PROP_FIRMWARE_UPDATE].handler.get     = firmware_update_to_bin;
    mdl_prop_list[OF_MDL_PROP_FIRMWARE_UPDATE].handler.get_alt = NULL; /* Not implemented : Write Only */
    mdl_prop_list[OF_MDL_PROP_FIRMWARE_UPDATE].handler.set_alt = NULL; /* Special File Based write elsewhere. */
    mdl_prop_list[OF_MDL_PROP_FIRMWARE_UPDATE].handler.compare = firmware_update_compare;

    mdl_prop_list[OF_MDL_PROP_COMMAND].handler.set     = NULL; /* Not implemented : Write Only */
    mdl_prop_list[OF_MDL_PROP_COMMAND].handler.get     = command_to_bin;
    mdl_prop_list[OF_MDL_PROP_COMMAND].handler.get_alt = NULL; /* Not implemented : Write Only */
    mdl_prop_list[OF_MDL_PROP_COMMAND].handler.set_alt = command_from_json;
    mdl_prop_list[OF_MDL_PROP_COMMAND].handler.compare = command_compare;

    mdl_prop_list[OF_MDL_PROP_MODULE_INFO].handler.set     = module_info_from_bin;
    mdl_prop_list[OF_MDL_PROP_MODULE_INFO].handler.get     = NULL; /* Not implemented : Read Only */
    mdl_prop_list[OF_MDL_PROP_MODULE_INFO].handler.get_alt = module_info_to_json;
    mdl_prop_list[OF_MDL_PROP_MODULE_INFO].handler.set_alt = NULL; /* Not implemented : Read Only */
    mdl_prop_list[OF_MDL_PROP_MODULE_INFO].handler.compare = NULL; /* Not implemented : Read Only */

    mdl_prop_list[OF_MDL_PROP_CHARACTER_SET].handler.set     = character_set_from_bin;
    mdl_prop_list[OF_MDL_PROP_CHARACTER_SET].handler.get     = character_set_to_bin;
    mdl_prop_list[OF_MDL_PROP_CHARACTER_SET].handler.get_alt = character_set_to_json;
    mdl_prop_list[OF_MDL_PROP_CHARACTER_SET].handler.set_alt = character_set_from_json;
    mdl_prop_list[OF_MDL_PROP_CHARACTER_SET].handler.compare = character_set_compare;

    mdl_prop_list[OF_MDL_PROP_CHARACTER].handler.set     = character_from_bin;
    mdl_prop_list[OF_MDL_PROP_CHARACTER].handler.get     = character_to_bin;
    mdl_prop_list[OF_MDL_PROP_CHARACTER].handler.get_alt = character_to_json;
    mdl_prop_list[OF_MDL_PROP_CHARACTER].handler.set_alt = character_from_json;
    mdl_prop_list[OF_MDL_PROP_CHARACTER].handler.compare = character_compare;

    mdl_prop_list[OF_MDL_PROP_OFFSET].handler.set     = offset_from_bin;
    mdl_prop_list[OF_MDL_PROP_OFFSET].handler.get     = offset_to_bin;
    mdl_prop_list[OF_MDL_PROP_OFFSET].handler.get_alt = offset_to_json;
    mdl_prop_list[OF_MDL_PROP_OFFSET].handler.set_alt = offset_from_json;
    mdl_prop_list[OF_MDL_PROP_OFFSET].handler.compare = offset_compare;

    mdl_prop_list[OF_MDL_PROP_COLOR].handler.set     = color_from_bin;
    mdl_prop_list[OF_MDL_PROP_COLOR].handler.get     = color_to_bin;
    mdl_prop_list[OF_MDL_PROP_COLOR].handler.get_alt = color_to_json;
    mdl_prop_list[OF_MDL_PROP_COLOR].handler.set_alt = color_from_json;
    mdl_prop_list[OF_MDL_PROP_COLOR].handler.compare = color_compare;

    mdl_prop_list[OF_MDL_PROP_MOTION].handler.set     = motion_from_bin;
    mdl_prop_list[OF_MDL_PROP_MOTION].handler.get     = motion_to_bin;
    mdl_prop_list[OF_MDL_PROP_MOTION].handler.get_alt = motion_to_json;
    mdl_prop_list[OF_MDL_PROP_MOTION].handler.set_alt = motion_from_json;
    mdl_prop_list[OF_MDL_PROP_MOTION].handler.compare = motion_compare;

    mdl_prop_list[OF_MDL_PROP_MINIMUM_ROTATION].handler.set     = min_rotation_from_bin;
    mdl_prop_list[OF_MDL_PROP_MINIMUM_ROTATION].handler.get     = min_rotation_to_bin;
    mdl_prop_list[OF_MDL_PROP_MINIMUM_ROTATION].handler.get_alt = min_rotation_to_json;
    mdl_prop_list[OF_MDL_PROP_MINIMUM_ROTATION].handler.set_alt = min_rotation_from_json;
    mdl_prop_list[OF_MDL_PROP_MINIMUM_ROTATION].handler.compare = min_rotation_compare;

    mdl_prop_list[OF_MDL_PROP_IR_THRESHOLD].handler.set     = ir_threshold_from_bin;
    mdl_prop_list[OF_MDL_PROP_IR_THRESHOLD].handler.get     = ir_threshold_to_bin;
    mdl_prop_list[OF_MDL_PROP_IR_THRESHOLD].handler.get_alt = ir_threshold_to_json;
    mdl_prop_list[OF_MDL_PROP_IR_THRESHOLD].handler.set_alt = ir_threshold_from_json;
    mdl_prop_list[OF_MDL_PROP_IR_THRESHOLD].handler.compare = ir_threshold_compare;
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

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Convert a json object into a color.
 *
 * \param[out] color The color to convert to.
 * \param[in] json The json object to convert.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static esp_err_t json_to_color(color_t *color, const cJSON *json)
{
    assert(color != NULL);
    assert(json != NULL);

    ESP_RETURN_ON_FALSE(cJSON_IsString(json), ESP_ERR_INVALID_ARG, "HANDLER", "Expected a string for color");

    const char *color_str = json->valuestring;

    ESP_RETURN_ON_FALSE(strlen(color_str) == 7, ESP_ERR_INVALID_ARG, "HANDLER", "Invalid color string length");

    ESP_RETURN_ON_FALSE(color_str[0] == '#', ESP_ERR_INVALID_ARG, "HANDLER", "Invalid color string format");

    uint32_t color_value = 0;
    ESP_RETURN_ON_FALSE(sscanf(color_str + 1, "%lx", &color_value), ESP_ERR_INVALID_ARG, TAG,
                        "Failed to convert color string");

    color->red   = (color_value >> 16) & 0xFF;
    color->green = (color_value >> 8) & 0xFF;
    color->blue  = color_value & 0xFF;

    return ESP_OK;
}