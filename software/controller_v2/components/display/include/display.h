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
} display_t;

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Initialize the display.
 *
 * \param[in] display The display to initialize.
 */
esp_err_t display_init(display_t *display);

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

/**
 * \brief Get the size of the display.
 *
 * \param[in] display The display to get the size of.
 *
 * \return The size of the display.
 */
uint16_t display_size_get(display_t *display);

/**
 * \brief Indicate that the model and modules are out of sync.
 *
 * \param[in] display The display to update.
 *
 * \retval ESP_OK on success.
 * \retval ESP_ERR_INVALID_ARG if display is NULL.
 */
esp_err_t display_event_desynchronized(display_t *display);

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

/**
 * \brief Indicate that the model and modules are in sync.
 *
 * \param[in] display The display to update.
 *
 * \retval ESP_OK on success.
 * \retval ESP_ERR_INVALID_ARG if display is NULL.
 */
esp_err_t display_event_synchronized(display_t *display);

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

/**
 * \brief Get a module from the display by the module index.
 *
 * \param[in] display The display to get the module from.
 * \param[in] module_index The index of the module to get.
 *
 * \return The module if found, NULL otherwise.
 */
module_t *display_module_get(display_t *display, uint16_t module_index);
