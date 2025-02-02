#include "module.h"
#include "esp_check.h"

#include <string.h>

#define TAG "MODULE"

esp_err_t module_character_set_index_of_character(module_t *module, uint8_t *index, const char *character)
{
    ESP_RETURN_ON_FALSE(index != NULL, ESP_ERR_INVALID_ARG, TAG, "Index is NULL");
    ESP_RETURN_ON_FALSE(module != NULL, ESP_ERR_INVALID_ARG, TAG, "Module is NULL");
    ESP_RETURN_ON_FALSE(module->character_set.size > 0, ESP_ERR_INVALID_ARG, TAG, "Character set size is 0");
    ESP_RETURN_ON_FALSE(module->character_set.character_set != NULL, ESP_ERR_INVALID_ARG, TAG, "Character set is NULL");

    const character_set_property_t *character_set = &module->character_set;

    for (int i = 0; i < character_set->size; i++) {
        if (strncmp(character, (const char *)&character_set->character_set[i * 4], 4) == 0) {
            *index = i;
            return ESP_OK;
        }
    }

    return ESP_FAIL;
}