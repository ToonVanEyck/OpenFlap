#include "esp_check.h"
#include "property_handler_common.h"

#include <string.h>

#define PROPERTY_TAG "COMMAND_PROPERTY_HANDLER"

/**
 * \brief Populate the property from a json object.
 *
 * \param[out] module The module containing the property.
 * \param[in] json The json object to convert.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static esp_err_t command_from_json(module_t *module, const cJSON *json)
{
    assert(module != NULL);
    assert(json != NULL);

    command_property_t *command = &module->command;

    ESP_RETURN_ON_FALSE(cJSON_IsString(json), ESP_ERR_INVALID_ARG, PROPERTY_TAG, "Expected a string");

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
 * \param[out] bin_size The size of the byte array.
 * \param[in] module The module containing the property.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static esp_err_t command_to_binary(uint8_t **bin, uint16_t *bin_size, const module_t *module)
{
    assert(bin != NULL);
    assert(bin_size != NULL);
    assert(module != NULL);

    const command_property_t *command = &module->command;

    *bin_size = chain_comm_property_read_attributes_get(PROPERTY_COMMAND)->static_property_size;

    *bin = malloc(sizeof(*bin_size));
    ESP_RETURN_ON_FALSE(*bin != NULL, ESP_ERR_NO_MEM, PROPERTY_TAG, "Failed to allocate memory");

    (*bin)[0] = *command;

    return ESP_OK;
}

/**
 * The command property handler.
 *
 * The command property is used to send commands to the modules. Commands are executed in the modules and do not store
 * any data, as such the property is write-only.
 */
const property_handler_t PROPERTY_HANDLER_COMMAND = {
    .id        = PROPERTY_COMMAND,
    .from_json = command_from_json,
    .to_binary = command_to_binary,
};

#undef PROPERTY_TAG