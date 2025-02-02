#include "module_property.h"
#include "esp_check.h"
#include "esp_log.h"
#include "module.h"

#include <string.h>

#define TAG "MODULE_PROPERTY"

esp_err_t module_property_indicate_desynchronized(module_t *module, property_id_t property_id)
{
    /* Validate inputs. */
    ESP_RETURN_ON_FALSE(module != NULL, ESP_ERR_INVALID_ARG, TAG, "Module is NULL");
    ESP_RETURN_ON_FALSE(property_id < PROPERTIES_MAX, ESP_ERR_INVALID_ARG, TAG, "Invalid property id");

    /* Indicate that the property has been desynchronized. */
    module->sync_properties_write_seq_required |= (1 << property_id);

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t module_property_indicate_synchronized(module_t *module, property_id_t property_id)
{
    /* Validate inputs. */
    ESP_RETURN_ON_FALSE(module != NULL, ESP_ERR_INVALID_ARG, TAG, "Module is NULL");
    ESP_RETURN_ON_FALSE(property_id < PROPERTIES_MAX, ESP_ERR_INVALID_ARG, TAG, "Invalid property id");

    /* Indicate that the property has been synchronized. */
    module->sync_properties_write_seq_required &= ~(1 << property_id);

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

bool module_property_is_desynchronized(module_t *module, property_id_t property_id)
{
    /* Validate inputs. */
    if (module == NULL) {
        ESP_LOGE(TAG, "Module is NULL");
        return false;
    }

    if (property_id >= PROPERTIES_MAX) {
        ESP_LOGE(TAG, "Invalid property id");
        return false;
    }

    return (module->sync_properties_write_seq_required & (1 << property_id));
}