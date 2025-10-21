#include "openflap_module.h"
#include "esp_check.h"
#include "openflap_properties.h"

#include <string.h>

#define TAG "MODULE"

esp_err_t module_character_set_index_of_character(module_t *module, uint8_t *index, const char *character)
{
    ESP_RETURN_ON_FALSE(index != NULL, ESP_ERR_INVALID_ARG, TAG, "Index is NULL");
    ESP_RETURN_ON_FALSE(module != NULL, ESP_ERR_INVALID_ARG, TAG, "Module is NULL");
    ESP_RETURN_ON_FALSE(module->character_set->size > 0, ESP_ERR_INVALID_ARG, TAG, "Character set size is 0");
    ESP_RETURN_ON_FALSE(module->character_set->data != NULL, ESP_ERR_INVALID_ARG, TAG, "Character set is NULL");

    for (int i = 0; i < module->character_set->size; i++) {
        if (strncmp(character, (const char *)&module->character_set->data[i * 4], 4) == 0) {
            *index = i;
            return ESP_OK;
        }
    }

    return ESP_FAIL;
}

module_t *module_new(void)
{
    module_t *module = malloc(sizeof(module_t));
    if (module == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for module");
        return NULL;
    }

    memset(module, 0, sizeof(module_t));

    return module;
}

void module_free(module_t *module)
{
    if (module == NULL) {
        return;
    }

    /* Free all properties. */
    firmware_version_free(module->firmware_version);
    firmware_update_free(module->firmware_update);
    character_set_free(module->character_set);

    free(module);
}

esp_err_t of_module_command_set(module_t *module, command_property_cmd_t command)
{
    ESP_RETURN_ON_FALSE(module != NULL, ESP_ERR_INVALID_ARG, TAG, "Module is NULL");

    module->command = command;

    return ESP_OK;
}

//----------------------------------------------------------------------------------------------------------------------------------

esp_err_t of_module_firmware_update_property_set(module_t *module, uint16_t index, const uint8_t *data)
{
    assert(module != NULL);
    assert(data != NULL);

    /* Create new firmware update property. */
    firmware_update_property_t *new_firmware_update = firmware_update_new();
    ESP_RETURN_ON_FALSE(new_firmware_update != NULL, ESP_ERR_NO_MEM, TAG, "Failed to allocate memory");

    /* Free the old data. */
    firmware_update_free(module->firmware_update);

    /* Update the module with the new firmware update. */
    module->firmware_update = new_firmware_update;

    /* Set the data. */
    module->firmware_update->index = index;
    memcpy(module->firmware_update->data, data, OF_FIRMWARE_UPDATE_PAGE_SIZE);

    return ESP_OK;
}