#pragma once

#include "esp_err.h"
#include "module_common.h"
#include "properties.h"

typedef enum {
    PROPERTY_SYNC_METHOD_READ,  /**< Property is synchronized by reading the actual module. */
    PROPERTY_SYNC_METHOD_WRITE, /**< Property is synchronized by writing to the actual module. */
} property_sync_method_t;

/**
 * \brief Get a property from a module by the property id.
 *
 * \param[in] module The module to get the property from.
 * \param[in] property_id The id of the property to get.
 *
 * \return The property if found, NULL otherwise.
 */
void *module_property_get_by_id(module_t *module, property_id_t property_id);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Get a property from a module by the property name.
 *
 * \param[in] module The module to get the property from.
 * \param[in] property_name The name of the property to get.
 *
 * \return The property if found, NULL otherwise.
 */
void *module_property_get_by_name(module_t *module, const char *property_name);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Set the properties of a module from a json object.
 *
 * \param[in] module The module to set the properties of.
 * \param[in] json The json object containing the properties.
 *
 * \return The number of successfully set properties.
 */
uint8_t module_properties_set_from_json(module_t *module, const cJSON *json);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Indicate that a property of a module has been updated and synchronisation between the module and model is
 * required.
 *
 * \param[in] module The module to indicate the property update of.
 * \param[in] property_id The id of the property that has been updated.
 * \param[in] sync_method The method of synchronization required.
 *
 * \retval ESP_OK The property has been indicated as desynchronized.
 * \retval ESP_ERR_INVALID_ARG The module is NULL or the property id is invalid.
 */
esp_err_t module_property_indicate_desynchronized(module_t *module, property_id_t property_id,
                                                  property_sync_method_t sync_method);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Indicate that a property of a module has been synchronized between the module and model.
 *
 * \param[in] module The module to indicate the property update of.
 * \param[in] property_id The id of the property that has been updated.
 *
 * \retval ESP_OK The property has been indicated as synchronized.
 * \retval ESP_ERR_INVALID_ARG The module is NULL or the property id is invalid.
 */
esp_err_t module_property_indicate_synchronized(module_t *module, property_id_t property_id);