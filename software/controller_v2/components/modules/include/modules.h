#pragma once

#include "properties.h"

/* Module types */
#include "splitflap_module.h"

/**
 * \brief Module structure.
 */
typedef struct {
    module_info_property_t module_info; /**< Module info property. */
    command_property_t command;         /**< Command property. */
    union {
        module_splitflap_t splitflap; /**< Splitflap module properties. */
    };
} module_t;

//---------------------------------------------------------------------------------------------------------------------

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