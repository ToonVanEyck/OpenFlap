#pragma once

#include "esp_check.h"
#include "property_handler_common.h"

#include <string.h>

#define PROPERTY_TAG "CHARACTER_SET_PROPERTY_HANDLER"

/**
 * \brief Populate the property from a json object.
 *
 * \param[in] json The json object to convert.
 * \param[out] property The property to populate.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static inline esp_err_t character_set_from_json(module_t *module, const cJSON *json)
{
    assert(module != NULL);
    assert(json != NULL);

    character_set_property_t *character_set = &module->character_set;

    /* Check if the character set has been initialized before. */
    ESP_RETURN_ON_FALSE(character_set->size > 0, ESP_ERR_INVALID_ARG, PROPERTY_TAG, "Character set size is 0");
    ESP_RETURN_ON_FALSE(character_set->character_set != NULL, ESP_ERR_INVALID_ARG, PROPERTY_TAG,
                        "Character set is NULL");

    /* Validate json. */
    ESP_RETURN_ON_FALSE(cJSON_IsArray(json), ESP_ERR_INVALID_ARG, PROPERTY_TAG, "Expected an array of strings");

    cJSON *character_set_entry = NULL;
    cJSON_ArrayForEach(character_set_entry, json)
    {
        ESP_RETURN_ON_FALSE(cJSON_IsString(character_set_entry), ESP_ERR_INVALID_ARG, PROPERTY_TAG,
                            "Expected an array of strings but got %s", character_set_entry->valuestring);
    }

    /* Get the character set size and allocate memory. */
    ESP_RETURN_ON_FALSE(character_set->size == cJSON_GetArraySize(json), ESP_ERR_INVALID_ARG, PROPERTY_TAG,
                        "Expected a character set of size %d", character_set->size);

    character_set->character_set = (uint8_t *)malloc(character_set->size * 4);
    ESP_RETURN_ON_FALSE(character_set->character_set != NULL, ESP_ERR_NO_MEM, PROPERTY_TAG, "Memory allocation failed");

    for (int i = 0; i < character_set->size; i++) {
        /* Get new entry from json. */
        character_set_entry = cJSON_GetArrayItem(json, i);
        /* Clear old entry. */
        memset((char *)(&character_set->character_set[i * 4]), 0, 4);
        /* Copy new entry. */
        strncpy((char *)(&character_set->character_set[i * 4]), character_set_entry->valuestring, 4);
    }

    return ESP_OK;
}

/**
 * \brief Convert the property into it's json representation.
 *
 * \param[out] json The json object in which we will store the property.
 * \param[in] property The property to convert.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static inline esp_err_t character_set_to_json(cJSON **json, const module_t *module)
{
    assert(json != NULL);
    assert(module != NULL);

    const character_set_property_t *character_set = &module->character_set;

    ESP_RETURN_ON_FALSE(character_set->size > 0, ESP_ERR_INVALID_ARG, PROPERTY_TAG, "Character set size is 0");

    *json = cJSON_CreateArray();
    ESP_RETURN_ON_FALSE(*json != NULL, ESP_ERR_NO_MEM, PROPERTY_TAG, "Memory allocation failed");

    for (int i = 0; i < character_set->size; i++) {
        cJSON *character_entry = cJSON_CreateString((char *)(&character_set->character_set[i * 4]));
        ESP_RETURN_ON_FALSE(character_entry != NULL, ESP_ERR_NO_MEM, PROPERTY_TAG, "Memory allocation failed");
        cJSON_AddItemToArray(*json, character_entry);
    }

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
static inline esp_err_t character_set_from_binary(module_t *module, const uint8_t *bin, uint16_t bin_size)
{
    assert(module != NULL);
    assert(bin != NULL);

    character_set_property_t *character_set = &module->character_set;

    ESP_RETURN_ON_FALSE(bin_size % 4 == 0, ESP_ERR_INVALID_ARG, PROPERTY_TAG,
                        "Invalid binary size, expected multiple of 4.");

    /* Allocate memory for the character set. */
    uint8_t *new_character_set = (uint8_t *)malloc(bin_size);
    ESP_RETURN_ON_FALSE(new_character_set != NULL, ESP_ERR_NO_MEM, PROPERTY_TAG, "Memory allocation failed");

    /* Free old character set. */
    if (character_set->character_set != NULL) {
        free(character_set->character_set);
        character_set->character_set = NULL;
    }

    character_set->size          = bin_size / 4;
    character_set->character_set = new_character_set;

    /* Copy the binary array to the character set. */
    memcpy(character_set->character_set, bin, character_set->size * 4);

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
static inline esp_err_t character_set_to_binary(uint8_t **bin, uint16_t *bin_size, const module_t *module)
{
    assert(bin != NULL);
    assert(bin_size != NULL);
    assert(module != NULL);

    const character_set_property_t *character_set = &module->character_set;

    /* Check if the character set has been initialized before. */
    ESP_RETURN_ON_FALSE(character_set->size > 0, ESP_ERR_INVALID_ARG, PROPERTY_TAG, "Character set size is 0");
    ESP_RETURN_ON_FALSE(character_set->character_set != NULL, ESP_ERR_INVALID_ARG, PROPERTY_TAG,
                        "Character set is NULL");

    *bin_size = character_set->size * 4;

    *bin = malloc(sizeof(*bin_size));
    ESP_RETURN_ON_FALSE(*bin != NULL, ESP_ERR_NO_MEM, PROPERTY_TAG, "Failed to allocate memory");

    /* Copy the character set to the binary array. */
    memcpy(*bin, character_set->character_set, character_set->size * 4);

    return ESP_OK;
}

/**
 * The character_set property handler.
 *
 * The character_set property is used to store the character_set data for the modules.
 */
static const property_handler_t PROPERTY_HANDLER_CHARACTER_SET = {
    .id          = PROPERTY_CHARACTER_SET,
    .from_json   = character_set_from_json,
    .to_json     = character_set_to_json,
    .from_binary = character_set_from_binary,
    .to_binary   = character_set_to_binary,
};

#undef PROPERTY_TAG