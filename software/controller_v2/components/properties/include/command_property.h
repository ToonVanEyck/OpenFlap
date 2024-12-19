#pragma once

#include "properties_common.h"

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
    ESP_LOGI("PROPERTY", "placeholder for: %s", __func__);
    return ESP_OK;
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
    ESP_LOGI("PROPERTY", "placeholder for: %s", __func__);
    return ESP_OK;
}

/**
 * The command property handler.
 *
 * The command property is used to send commands to the modules. Commands are executed in the modules and do not store
 * any data, as such the property is write-only.
 */
static const property_handler_t PROPERTY_HANDLER_COMMAND = {
    .id = PROPERTY_COMMAND, /*.name = "command",*/ .from_json = command_from_json, .to_binary = command_to_binary,
    // .to_binary_attributes = {.static_size = 1},
};