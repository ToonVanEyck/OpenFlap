#pragma once

#include "esp_err.h"
#include "module.h"

/**
 * \brief Set the command property of a module.
 *
 * \param[in] module The module to set the property of.
 * \param[in] command The command to set.
 *
 * \return esp_err_t
 */
esp_err_t property_handler_command_set(module_t *module, command_property_cmd_t command);