#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_log.h"
#include "openflap_cc_master.h"
#include "openflap_module.h"

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

/**
 * \brief Display structure.
 */
typedef struct {
    of_cc_master_ctx_t cc_master; /**< Openflap Chain communication master context. */

    module_t **modules;    /**< Array of modules. */
    uint16_t module_count; /**< Number of modules. */

    /** Indicates which properties need to be synchronized by reading actual modules. */
    uint64_t sync_prop_read_required;
    /** Indicates which properties need to be synchronized by writing to actual modules. */
    uint64_t sync_prop_write_required;
} of_display_t;

typedef enum {
    PROPERTY_SYNC_METHOD_READ,  /**< Property is synchronized by reading the actual module. */
    PROPERTY_SYNC_METHOD_WRITE, /**< Property is synchronized by writing to the actual module. */
} property_sync_method_t;

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Initialize the display.
 *
 * \param[in] display The display to initialize.
 */
esp_err_t of_display_init(of_display_t *display);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Destroy the display.
 *
 * \param[in] display The display to destroy.
 */
esp_err_t display_destroy(of_display_t *display);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Get the size of the display.
 *
 * \param[in] display The display to get the size of.
 *
 * \return The size of the display.
 */
uint16_t display_size_get(of_display_t *display);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Synchronize the display with the actual modules.
 *
 * \param[in] display The display to synchronize.
 * \param[in] timeout_ms The timeout in milliseconds to wait for synchronization. (0 for no wait)
 */
esp_err_t of_display_synchronize(of_display_t *display, uint32_t timeout_ms);

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Get a module from the display by the module index.
 *
 * \param[in] display The display to get the module from.
 * \param[in] module_index The index of the module to get.
 *
 * \return The module if found, NULL otherwise.
 */
module_t *display_module_get(of_display_t *display, uint16_t module_index);

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Indicate that a property of all modules has been updated and synchronisation between the display and model is
 * required.
 *
 * \param[in] display The display of which the properties have been updated.
 * \param[in] property_id The id of the property that has been updated.
 * \param[in] sync_method The method of synchronization required.
 *
 * \retval ESP_OK The property has been indicated as desynchronized.
 * \retval ESP_ERR_INVALID_ARG The display is NULL or the property id is invalid.
 */
esp_err_t display_property_indicate_desynchronized(of_display_t *display, cc_prop_id_t property_id,
                                                   property_sync_method_t sync_method);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Indicate that a property of all modules has been synchronized between the display and model.
 *
 * \param[in] display The display of which the properties have been synchronized.
 * \param[in] property_id The id of the property that has been synchronized.
 *
 * \retval ESP_OK The property has been indicated as synchronized.
 * \retval ESP_ERR_INVALID_ARG The display is NULL or the property id is invalid.
 */
esp_err_t display_property_indicate_synchronized(of_display_t *display, cc_prop_id_t property_id);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Promote a property from write sequence to write all.
 *
 * If the property of any number of modules is required, but the property is equal for all modules, the property can be
 * promoted to write all. This will allow the chain communication to write the property to all modules at once.
 *
 * \param[in] display The display ctx.
 */
void display_property_promote_write_seq_to_write_all(of_display_t *display);