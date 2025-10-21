#pragma once

#include "openflap_module_property.h"

/**
 * \brief Get the index of a character in the modules character set.
 *
 * \param[in] module The module to get the character index from.
 * \param[out] index The index of the character in the character set.
 * \param[in] character The character to get the index of, this is a 4 byte array to support UTF-8.
 *
 * \return ESP_OK if the index of the character has been found.
 */
esp_err_t module_character_set_index_of_character(module_t *module, uint8_t *index, const char *character);

/**
 * \brief Create a new module.
 *
 * \return A new module.
 */
module_t *module_new(void);

/**
 * \brief Destroy a module.
 *
 * \param[in] module The module to destroy.
 */
void module_free(module_t *module);

/**
 * \brief Set the command property of a module.
 *
 * \param[in] module The module to set the property of.
 * \param[in] command The command to set.
 *
 * \return esp_err_t
 */
esp_err_t of_module_command_set(module_t *module, command_property_cmd_t command);

esp_err_t of_module_firmware_update_property_set(module_t *module, uint16_t index, const uint8_t *data);
