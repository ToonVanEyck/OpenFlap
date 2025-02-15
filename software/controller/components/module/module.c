#include "module.h"
#include "esp_check.h"

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