#pragma once

#include "properties_common.h"

/** Enum with supported commands. */
typedef enum {
    no_command,     /**< No command. */
    reboot_command, /**< Reboot the modules. */
} command_property_t;

/**
 * \brief Populate the property from a json object.
 *
 * \param[out] property The property to populate.
 * \param[in] json The json object to convert.
 *
 * \return true if the conversion was successful, false otherwise.
 */
static inline bool command_from_json(void *property, const cJSON *json)
{
    ESP_LOGI("PROPERTY", "placeholder for: %s", __func__);
    return true;
}

/**
 * \brief Serialize the property into a byte array.
 *
 * \param[out] bin The serialized byte array.
 * \param[in] property The property to serialize.
 * \param[in] index The index of in case of a multipart property.
 *
 * \return true if the conversion was successful, false otherwise.
 */
static inline bool command_to_binary(uint8_t *bin, const void *property, uint8_t index)
{
    ESP_LOGI("PROPERTY", "placeholder for: %s", __func__);
    return true;
}

/**
 * The command property handler.
 *
 * The command property is used to send commands to the modules. Commands are executed in the modules and do not store
 * any data, as such the property is write-only.
 */
static const property_handler_t PROPERTY_HANDLER_COMMAND = {
    .id                   = PROPERTY_COMMAND,
    .name                 = "command",
    .from_json            = command_from_json,
    .to_binary            = command_to_binary,
    .to_binary_attributes = {.static_size = 1},
};