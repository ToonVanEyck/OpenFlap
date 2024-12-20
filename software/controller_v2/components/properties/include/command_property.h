#pragma once

#include "esp_check.h"
#include "properties_common.h"
#include <string.h>

#define PROPERTY_TAG "COMMAND_PROPERTY_HANDLER"

/**
 * \brief Populate the property from a json object.
 *
 * \param[out] property The property to populate.
 * \param[in] json The json object to convert.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static inline esp_err_t command_from_json(void *property, const cJSON *json)
{
    assert(property != NULL);
    assert(json != NULL);

    command_property_t *command = (command_property_t *)property;

    ESP_RETURN_ON_FALSE(json->type != cJSON_String, ESP_ERR_INVALID_ARG, PROPERTY_TAG, "Expected a string");

    for (command_property_t cmd = CMD_NO_COMMAND + 1; cmd < CMD_MAX; cmd++) {
        if (strcmp(json->valuestring, chain_comm_command_name_get(cmd)) == 0) {
            *command = cmd;
            return ESP_OK;
        }
    }

    ESP_LOGE(PROPERTY_TAG, "Command \"%s\" is not supported.", json->valuestring);
    return ESP_ERR_INVALID_ARG;
}

/**
 * \brief Serialize the property into a byte array.
 *
 * \param[out] bin The serialized byte array.
 * \param[in] property The property to serialize.
 * \param[in] index The index of in case of a multipart property.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static inline esp_err_t command_to_binary(uint8_t *bin, const void *property, uint8_t index)
{
    assert(bin != NULL);
    assert(property != NULL);

    const command_property_t *command = (const command_property_t *)property;

    bin[0] = *command;

    return ESP_OK;
}

/**
 * The command property handler.
 *
 * The command property is used to send commands to the modules. Commands are executed in the modules and do not store
 * any data, as such the property is write-only.
 */
static const property_handler_t PROPERTY_HANDLER_COMMAND = {
    .id        = PROPERTY_COMMAND,
    .from_json = command_from_json,
    .to_binary = command_to_binary,
};

#undef PROPERTY_TAG