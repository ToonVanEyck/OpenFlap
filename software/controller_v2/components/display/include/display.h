#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_log.h"
#include "module.h"
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

/**
 * \brief Display structure.
 */
typedef struct {
    module_t *modules;               /**< Array of modules. */
    uint16_t module_count;           /**< Number of modules. */
    EventGroupHandle_t event_handle; /**< Display event group handle. */
    /** Indicates witch properties need to be synchronized by reading actual modules. */
    uint64_t sync_properties_read_all_required;
    /** Indicates witch properties need to be synchronized by writing to actual modules. */
    uint64_t sync_properties_write_all_required;
} display_t;

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
esp_err_t display_init(display_t *display);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Resize the display.
 *
 * \param[in] display The display to resize.
 * \param[in] module_count The new number of modules.
 *
 * \retval ESP_OK on success.
 * \retval ESP_ERR_INVALID_ARG if display is NULL.
 * \retval ESP_ERR_NO_MEM if memory allocation fails.
 */
esp_err_t display_resize(display_t *display, uint16_t module_count);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Get the size of the display.
 *
 * \param[in] display The display to get the size of.
 *
 * \return The size of the display.
 */
uint16_t display_size_get(display_t *display);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Indicate that the model and modules are out of sync.
 *
 * \param[in] display The display to update.
 *
 * \retval ESP_OK on success.
 * \retval ESP_ERR_INVALID_ARG if display is NULL.
 */
esp_err_t display_event_desynchronized(display_t *display);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Wait for the display to be desynchronised.
 *
 * \param[in] display The display to wait for.
 * \param[in] ticks_to_wait The maximum time to wait for the display to be updated.
 *
 * \retval ESP_ERR_INVALID_ARG if display is NULL.
 * \retval ESP_OK when the display desynchronize event occurs.
 * \retval ESP_FAIL when a display failed event occurs the operation timed out.
 */
esp_err_t display_event_wait_for_desynchronized(display_t *display, TickType_t ticks_to_wait);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Indicate that the model and modules are in sync.
 *
 * \param[in] display The display to update.
 *
 * \retval ESP_OK on success.
 * \retval ESP_ERR_INVALID_ARG if display is NULL.
 */
esp_err_t display_event_synchronized(display_t *display);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Wait for the display to be synchronized.
 *
 * \param[in] display The display to wait for.
 * \param[in] ticks_to_wait The maximum time to wait for the display to be updated.
 *
 * \retval ESP_ERR_INVALID_ARG if display is NULL.
 * \retval ESP_OK when the display synchronize event occurs.
 * \retval ESP_FAIL when a display failed event occurs the operation timed out.
 */
esp_err_t display_event_wait_for_synchronized(display_t *display, TickType_t ticks_to_wait);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Get a module from the display by the module index.
 *
 * \param[in] display The display to get the module from.
 * \param[in] module_index The index of the module to get.
 *
 * \return The module if found, NULL otherwise.
 */
module_t *display_module_get(display_t *display, uint16_t module_index);

//---------------------------------------------------------------------------------------------------------------------

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
esp_err_t display_property_indicate_desynchronized(display_t *display, property_id_t property_id,
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
esp_err_t display_property_indicate_synchronized(display_t *display, property_id_t property_id);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Check if a property of a module is desynchronized.
 *
 * \param[in] display The display to check.
 * \param[in] property_id The id of the property to check.
 * \param[in] sync_method The method of synchronization to check.
 *
 * \return True if the property is desynchronized, false otherwise.
 */
bool display_property_is_desynchronized(display_t *display, property_id_t property_id,
                                        property_sync_method_t sync_method);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Promote a property from write sequence to write all.
 *
 * If the property of any number of modules is required, but the property is equal for all modules, the property can be
 * promoted to write all. This will allow the chain communication to write the property to all modules at once.
 *
 * \param[in] display The display ctx.
 */
void display_property_promote_write_seq_to_write_all(display_t *display);