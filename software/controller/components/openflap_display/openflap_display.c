#include "openflap_display.h"
#include "esp_check.h"
#include "esp_log.h"
#include "openflap_property_handlers.h"

#include <string.h>

//======================================================================================================================
//                                                   MACROS a DEFINES
//======================================================================================================================

#define TAG "DISPLAY"

//======================================================================================================================
//                                                   FUNCTION PROTOTYPES
//======================================================================================================================

static bool of_display_resize(void *model_userdata, uint16_t module_count);
static bool of_display_module_exists_and_must_be_written(void *model_userdata, uint16_t node_idx,
                                                         cc_prop_id_t property_id, bool *must_be_written);
static void of_display_module_error_set(void *model_userdata, uint16_t node_idx, cc_node_err_t error,
                                        cc_node_state_t state);
static cc_action_t of_display_prop_sync_required(void *model_userdata, cc_prop_id_t property_id);
static void of_display_prop_sync_done(void *model_userdata, cc_prop_id_t property_id);

//======================================================================================================================
//                                                   PUBLIC FUNCTIONS
//======================================================================================================================

esp_err_t of_display_init(of_display_t *display)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "Display is NULL");

    /* Clear memory. */
    memset(display, 0, sizeof(of_display_t));

    of_cc_master_cb_cfg_t of_cc_master_cb_cfg = {
        .node_cnt_update                 = of_display_resize,
        .node_exists_and_must_be_written = of_display_module_exists_and_must_be_written,
        .node_error_set                  = of_display_module_error_set,
        .model_sync_required             = of_display_prop_sync_required,
        .model_sync_done                 = of_display_prop_sync_done,
    };

    ESP_RETURN_ON_ERROR(of_cc_master_init(&display->cc_master, display, &of_cc_master_cb_cfg), TAG,
                        "Failed to initialize CC master");

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t display_destroy(of_display_t *display)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "Display is NULL");

    /* Free modules. */
    return of_display_resize(display, 0) ? ESP_OK : ESP_FAIL;
}

//---------------------------------------------------------------------------------------------------------------------

uint16_t display_size_get(of_display_t *display)
{
    ESP_RETURN_ON_FALSE(display != NULL, 0, TAG, "Display is NULL");

    return display->module_count;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t of_display_synchronize(of_display_t *display, uint32_t timeout_ms)
{
    return of_cc_master_synchronize(&display->cc_master, timeout_ms);
}

//---------------------------------------------------------------------------------------------------------------------

module_t *display_module_get(of_display_t *display, uint16_t module_index)
{
    if ((display == NULL) || (module_index >= display->module_count)) {
        ESP_LOGE(TAG, "Invalid display or module index: %d", module_index);
        return NULL;
    }
    return display->modules[module_index];
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t display_property_indicate_desynchronized(of_display_t *display, cc_prop_id_t property_id,
                                                   property_sync_method_t sync_method)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "Display is NULL");
    ESP_RETURN_ON_FALSE(property_id < OF_CC_PROP_CNT, ESP_ERR_INVALID_ARG, TAG, "Invalid property id");

    /* Reset all flags synchronisation flags because reads and writes cannot be combined. */
    // display_property_indicate_synchronized(display, property_id);

    /* Indicate that all modules have been desynchronised. */
    if (sync_method == PROPERTY_SYNC_METHOD_READ) {
        display->sync_prop_read_required |= (1 << property_id);
    } else {
        display->sync_prop_write_required |= (1 << property_id);
    }

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t display_property_indicate_synchronized(of_display_t *display, cc_prop_id_t property_id)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "Display is NULL");
    ESP_RETURN_ON_FALSE(property_id < OF_CC_PROP_CNT, ESP_ERR_INVALID_ARG, TAG, "Invalid property id");

    /* Reset the synchronisation flags for display. */
    display->sync_prop_read_required &= ~(1 << property_id);
    display->sync_prop_write_required &= ~(1 << property_id);

    /* Reset the synchronization flags for all modules if required. */
    if (display->sync_prop_write_required_broadcast_possible & (1 << property_id)) {
        display->sync_prop_write_required_broadcast_possible &= ~(1 << property_id);
        for (uint16_t i = 0; i < display_size_get(display); i++) {
            module_property_indicate_synchronized(display_module_get(display, i), property_id);
        }
    }

    return ESP_OK;
}

//----------------------------------------------------------------------------------------------------------------------

void display_property_promote_write_seq_to_write_all(of_display_t *display)
{
    if (display_size_get(display) == 0) {
        return;
    }

    for (cc_prop_id_t prop_id = 0; prop_id < OF_CC_PROP_CNT; prop_id++) {

        if (!(display->sync_prop_write_required & (1 << prop_id))) {
            continue; // Property is not marked for writing.
        }

        /* Check if the property can be compared. */
        if (cc_prop_list[prop_id].handler.compare == NULL) {
            continue;
        }

        /* Get the serialized data for the first module. */
        bool all_modules_are_same    = true;
        module_t *module_a           = display_module_get(display, 0);
        bool module_property_updated = module_property_is_desynchronized(module_a, prop_id);

        /* Compare the serialized data for the next properties. */
        for (uint16_t i = 1; all_modules_are_same && i < display_size_get(display); i++) {
            module_t *module_b = display_module_get(display, i);
            module_property_updated |= module_property_is_desynchronized(module_b, prop_id);
            all_modules_are_same &= cc_prop_list[prop_id].handler.compare(module_a, module_b);
        }

        /* If all modules are the same and the property has been updated, promote the write all. */
        if (all_modules_are_same && module_property_updated) {
            ESP_LOGI(TAG, "Promoting [%s] property to write all", cc_prop_list[prop_id].attribute.name);
            display_property_indicate_desynchronized(display, prop_id, PROPERTY_SYNC_METHOD_WRITE);
            for (uint16_t i = 0; i < display_size_get(display); i++) {
                module_a = display_module_get(display, i);
                module_property_indicate_synchronized(module_a, prop_id);
            }
        }
    }
}

//======================================================================================================================
//                                                         PRIVATE FUNCTIONS
//======================================================================================================================

static bool of_display_resize(void *model_userdata, uint16_t module_count)
{
    of_display_t *display = (of_display_t *)model_userdata;

    ESP_RETURN_ON_FALSE(display != NULL, false, TAG, "Display is NULL");

    if (module_count == display->module_count) {
        return true; /* No resize required. */
    }

    ESP_LOGI(TAG, "Resizing display from %d to %d modules", display->module_count, module_count);

    /* Free the modules which will be removed by the realloc. */
    for (uint16_t i = module_count; i < display->module_count; i++) {
        module_free(display->modules[i]);
    }

    /* Reallocate memory for the modules. */
    module_t **new_modules = NULL;
    if (module_count == 0) {
        free(display->modules);
    } else {
        new_modules = realloc(display->modules, module_count * sizeof(module_t *));
        ESP_RETURN_ON_FALSE(new_modules != NULL, false, TAG, "Failed to reallocate memory for modules");
    }

    /* Allocate the new modules after the realloc. */
    for (uint16_t i = display->module_count; i < module_count; i++) {
        new_modules[i] = module_new();
    }

    display->modules      = new_modules;
    display->module_count = module_count;

    return true;
}

//----------------------------------------------------------------------------------------------------------------------

static bool of_display_module_exists_and_must_be_written(void *model_userdata, uint16_t node_idx,
                                                         cc_prop_id_t property_id, bool *must_be_written)
{
    of_display_t *display = (of_display_t *)model_userdata;

    module_t *module = display_module_get(display, node_idx);
    if (module == NULL) {
        *must_be_written = false;
        return false;
    }

    *must_be_written = module_property_is_desynchronized(module, property_id);
    return true;
}

//----------------------------------------------------------------------------------------------------------------------

static void of_display_module_error_set(void *model_userdata, uint16_t node_idx, cc_node_err_t error,
                                        cc_node_state_t state)
{
    // of_display_t *display = (of_display_t *)model_userdata;
    (void)model_userdata;

    ESP_LOGE(TAG, "Module %d error: %s, state: %s", node_idx, cc_node_error_to_str(error), cc_node_state_to_str(state));
}

//----------------------------------------------------------------------------------------------------------------------

static cc_action_t of_display_prop_sync_required(void *model_userdata, cc_prop_id_t property_id)
{
    of_display_t *display = (of_display_t *)model_userdata;

    /* It should not be possible that a property is both read and written at the same time. */

    if (display->sync_prop_read_required & (1 << property_id)) {
        return CC_ACTION_READ;
    } else if (display->sync_prop_write_required & (1 << property_id)) {
        return CC_ACTION_WRITE;
    } else if (display->sync_prop_write_required_broadcast_possible & (1 << property_id)) {
        return CC_ACTION_BROADCAST;
    }
    return -1; /* No synchronization required. */
}

//----------------------------------------------------------------------------------------------------------------------

static void of_display_prop_sync_done(void *model_userdata, cc_prop_id_t property_id)
{
    of_display_t *display = (of_display_t *)model_userdata;
    display_property_indicate_synchronized(display, property_id);
}

//----------------------------------------------------------------------------------------------------------------------