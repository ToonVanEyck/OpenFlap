#include "module_property.h"
#include "esp_check.h"
#include "esp_log.h"
#include "module.h"

#include <string.h>

#define TAG "MODULE_PROPERTY"

esp_err_t module_property_indicate_desynchronized(module_t *module, cc_prop_id_t property_id)
{
    /* Validate inputs. */
    ESP_RETURN_ON_FALSE(module != NULL, ESP_ERR_INVALID_ARG, TAG, "Module is NULL");
    ESP_RETURN_ON_FALSE(property_id < PROPERTIES_MAX, ESP_ERR_INVALID_ARG, TAG, "Invalid property id");

    /* Indicate that the property has been desynchronized. */
    module->sync_properties_write_seq_required |= (1 << property_id);

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t module_property_indicate_synchronized(module_t *module, cc_prop_id_t property_id)
{
    /* Validate inputs. */
    ESP_RETURN_ON_FALSE(module != NULL, ESP_ERR_INVALID_ARG, TAG, "Module is NULL");
    ESP_RETURN_ON_FALSE(property_id < PROPERTIES_MAX, ESP_ERR_INVALID_ARG, TAG, "Invalid property id");

    /* Indicate that the property has been synchronized. */
    module->sync_properties_write_seq_required &= ~(1 << property_id);

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

bool module_property_is_desynchronized(module_t *module, cc_prop_id_t property_id)
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

//----------------------------------------------------------------------------------------------------------------------------------

firmware_version_property_t *firmware_version_new(size_t size)
{
    /* Allocate the property. */
    firmware_version_property_t *firmware_version = malloc(sizeof(firmware_version_property_t));
    if (firmware_version == NULL) {
        return NULL;
    }

    /* Initialize the reference count. */
    firmware_version->reff_cnt = 1;

    /* Allocate the data. */
    firmware_version->str = calloc(size + 1, 1);
    if (firmware_version->str == NULL) {
        free(firmware_version);
        return NULL;
    }

    return firmware_version;
}

//----------------------------------------------------------------------------------------------------------------------------------

void firmware_version_free(firmware_version_property_t *firmware_version)
{
    /* Don't do anything if the pointer is NULL. */
    if (firmware_version == NULL) {
        return;
    }

    /* Reduce reference count. */
    if (firmware_version->reff_cnt > 1) {
        firmware_version->reff_cnt--;
        return;
    }

    /* Free the data when count reaches 0. */
    free(firmware_version->str);
    free(firmware_version);
}

//----------------------------------------------------------------------------------------------------------------------------------

firmware_update_property_t *firmware_update_new(void)
{
    /* Allocate the property. */
    firmware_update_property_t *firmware_update = malloc(sizeof(firmware_update_property_t));
    if (firmware_update == NULL) {
        return NULL;
    }

    /* Initialize the reference count. */
    firmware_update->reff_cnt = 1;

    /* Allocate the data. */
    uint16_t size = chain_comm_property_write_attributes_get(PROPERTY_FIRMWARE_UPDATE)->static_property_size - 2;
    firmware_update->data = calloc(size, 1);
    if (firmware_update->data == NULL) {
        free(firmware_update);
        return NULL;
    }

    return firmware_update;
}

//----------------------------------------------------------------------------------------------------------------------------------

void firmware_update_free(firmware_update_property_t *firmware_update)
{
    /* Don't do anything if the pointer is NULL. */
    if (firmware_update == NULL) {
        return;
    }

    /* Reduce reference count. */
    if (firmware_update->reff_cnt > 1) {
        firmware_update->reff_cnt--;
        return;
    }

    /* Free the data when count reaches 0. */
    free(firmware_update->data);
    free(firmware_update);
}

//----------------------------------------------------------------------------------------------------------------------------------

character_set_property_t *character_set_new(uint8_t size)
{
    /* Allocate the property. */
    character_set_property_t *character_set = malloc(sizeof(character_set_property_t));
    if (character_set == NULL) {
        return NULL;
    }

    /* Initialize the reference count. */
    character_set->reff_cnt = 1;

    /* Allocate the data. */
    character_set->size = size;
    character_set->data = calloc(size, 4);
    if (character_set->data == NULL) {
        free(character_set);
        return NULL;
    }

    return character_set;
}

//----------------------------------------------------------------------------------------------------------------------------------

void character_set_free(character_set_property_t *character_set)
{
    /* Don't do anything if the pointer is NULL. */
    if (character_set == NULL) {
        return;
    }

    /* Reduce reference count. */
    if (character_set->reff_cnt > 1) {
        character_set->reff_cnt--;
        return;
    }

    /* Free the data when count reaches 0. */
    free(character_set->data);
    free(character_set);
}