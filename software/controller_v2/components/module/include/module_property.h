#pragma once

#include "esp_err.h"
#include "module_common.h"

/**
 * \brief Indicate that a property of a module has been updated and and needs to be written to the actual module.
 *
 * \param[in] module The module which has it's property updated.
 * \param[in] property_id The id of the property that has been updated.
 *
 * \retval ESP_OK The property has been indicated as desynchronized.
 * \retval ESP_ERR_INVALID_ARG The module is NULL or the property id is invalid.
 */
esp_err_t module_property_indicate_desynchronized(module_t *module, property_id_t property_id);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Indicate that a property of a modules has been synchronized between the display and model.
 *
 * \param[in] display The module of which the property has been synchronized.
 * \param[in] property_id The id of the property that has been synchronized.
 *
 * \retval ESP_OK The property has been indicated as synchronized.
 * \retval ESP_ERR_INVALID_ARG The display is NULL or the property id is invalid.
 */
esp_err_t module_property_indicate_synchronized(module_t *module, property_id_t property_id);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Check if a property of a module is desynchronized.
 *
 * \param[in] module The module to check.
 * \param[in] property_id The id of the property to check.
 *
 * \retval true The property is desynchronized.
 * \retval false The property is synchronized.
 */
bool module_property_is_desynchronized(module_t *module, property_id_t property_id);
