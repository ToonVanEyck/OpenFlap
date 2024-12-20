#include "display.h"
#include "esp_check.h"
#include "esp_log.h"

#define TAG "DISPLAY"

/** Indicates that the model and actual modules are no longer in sync. */
#define DISPLAY_EVENT_DESYNCHRONIZED (1u << 0)
/** Indicates that the model and actual modules are back in sync. */
#define DISPLAY_EVENT_SYNCHRONIZED (1u << 1)
/** Indicates that the action preformed on the display has failed. */
#define DISPLAY_EVENT_FAILED (1u << 2)

//---------------------------------------------------------------------------------------------------------------------

esp_err_t display_init(display_t *display)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "Display is NULL");

    display->modules      = NULL;
    display->module_count = 0;

    display->event_handle = xEventGroupCreate();
    ESP_RETURN_ON_FALSE(display->event_handle != NULL, ESP_FAIL, TAG, "Failed to create event group for display");

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t display_resize(display_t *display, uint16_t module_count)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "Display is NULL");

    /* Reallocate memory for the modules. */
    module_t *new_modules = realloc(display->modules, module_count * sizeof(module_t));
    ESP_RETURN_ON_FALSE(new_modules != NULL, ESP_ERR_NO_MEM, TAG, "Failed to reallocate memory for modules");

    display->modules      = new_modules;
    display->module_count = module_count;

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

uint16_t display_size_get(display_t *display)
{
    ESP_RETURN_ON_FALSE(display != NULL, 0, TAG, "Display is NULL");

    return display->module_count;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t display_event_desynchronized(display_t *display)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "Display is NULL");

    /* Set the updated event. */
    xEventGroupSetBits(display->event_handle, DISPLAY_EVENT_DESYNCHRONIZED);

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t display_event_wait_for_desynchronized(display_t *display, TickType_t ticks_to_wait)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "Display is NULL");

    EventBits_t bits = xEventGroupWaitBits(display->event_handle, DISPLAY_EVENT_DESYNCHRONIZED | DISPLAY_EVENT_FAILED,
                                           pdTRUE, pdFALSE, ticks_to_wait);

    if ((bits & DISPLAY_EVENT_DESYNCHRONIZED) == 0) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t display_event_synchronized(display_t *display)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "Display is NULL");

    /* Set the updated event. */
    xEventGroupSetBits(display->event_handle, DISPLAY_EVENT_SYNCHRONIZED);

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t display_event_wait_for_synchronized(display_t *display, TickType_t ticks_to_wait)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "Display is NULL");

    EventBits_t bits = xEventGroupWaitBits(display->event_handle, DISPLAY_EVENT_SYNCHRONIZED | DISPLAY_EVENT_FAILED,
                                           pdTRUE, pdFALSE, ticks_to_wait);

    if ((bits & DISPLAY_EVENT_SYNCHRONIZED) == 0) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

module_t *display_module_get(display_t *display, uint16_t module_index)
{
    if ((display == NULL) || (module_index >= display->module_count)) {
        ESP_LOGE(TAG, "Invalid display or module index: %d", module_index);
        return NULL;
    }
    return &display->modules[module_index];
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t display_property_indicate_desynchronized(display_t *display, property_id_t property_id,
                                                   property_sync_method_t sync_method)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "Display is NULL");
    ESP_RETURN_ON_FALSE(property_id < PROPERTIES_MAX, ESP_ERR_INVALID_ARG, TAG, "Invalid property id");

    /* Reset all flags synchronisation flags because reads and writes cannot be combined. */
    display_property_indicate_synchronized(display, property_id);

    /* Indicate that all modules have been desynchronised. */
    if (sync_method == PROPERTY_SYNC_METHOD_READ) {
        display->sync_properties_read_all_required |= (1 << property_id);
    } else {
        display->sync_properties_write_all_required |= (1 << property_id);
    }

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t display_property_indicate_synchronized(display_t *display, property_id_t property_id)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "Display is NULL");
    ESP_RETURN_ON_FALSE(property_id < PROPERTIES_MAX, ESP_ERR_INVALID_ARG, TAG, "Invalid property id");

    /* Reset the synchronisation flags for display. */
    display->sync_properties_write_all_required &= ~(1 << property_id);
    display->sync_properties_read_all_required &= ~(1 << property_id);

    /* Indicate that all modules have been synchronized. */
    for (uint16_t i = 0; i < display_size_get(display); i++) {
        module_property_indicate_synchronized(display_module_get(display, i), property_id);
    }

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

bool display_property_is_desynchronized(display_t *display, property_id_t property_id,
                                        property_sync_method_t sync_method)
{
    /* Validate inputs. */
    if (display == NULL) {
        ESP_LOGE(TAG, "Module is NULL");
        return false;
    }

    if (property_id >= PROPERTIES_MAX) {
        ESP_LOGE(TAG, "Invalid property id");
        return false;
    }

    if (sync_method == PROPERTY_SYNC_METHOD_READ) {
        return (display->sync_properties_read_all_required & (1 << property_id));
    }
    /*else*/
    return (display->sync_properties_write_all_required & (1 << property_id));
}