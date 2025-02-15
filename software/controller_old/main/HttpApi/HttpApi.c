#include "HttpApi.h"
#include "flap_http_server.h"

static const char *TAG = "[HTTP_API]";

#define GUARD(_condition, _message, ...)                                                                               \
    if (_condition) {                                                                                                  \
        ESP_LOGW(TAG, _message, ##__VA_ARGS__);                                                                        \
        return false;                                                                                                  \
    }

bool columnEnd_toJson(cJSON **json, module_t *module)
{
    *json = cJSON_CreateBool(module_getColumnEnd(module));
    return true;
}

bool character_toJson(cJSON **json, module_t *module)
{
    *json = cJSON_CreateString(module_getCharacter(module));
    return true;
}

bool character_fromJson(cJSON **json, module_t *module)
{
    GUARD(!cJSON_IsString((*json)), "property must be a string");
    module_setCharacter(module, (*json)->valuestring);
    return true;
}

bool characterMapSize_toJson(cJSON **json, module_t *module)
{
    *json = cJSON_CreateNumber(module_getCharacterMap(module)->size);
    return true;
}

bool characterMap_toJson(cJSON **json, module_t *module)
{
    *json                        = cJSON_CreateArray();
    characterMap_t *characterMap = module_getCharacterMap(module);
    char character[5]            = {0};
    for (int i = 0; i < characterMap->size; i++) {
        characterMap_getCharacter(characterMap, i, character);
        cJSON *characterMapMember = cJSON_CreateString(character);
        cJSON_AddItemToArray(*json, characterMapMember);
    }
    return true;
}

bool characterMap_fromJson(cJSON **json, module_t *module)
{
    GUARD(!(cJSON_IsArray(*json)), "property must be an array");
    characterMap_t *characterMap = characterMap_new(48);
    GUARD(!characterMap, "Failed to create a new characterMap");
    cJSON *map_it = NULL;
    int i         = 0;
    cJSON_ArrayForEach(map_it, *json)
    {
        if (!cJSON_IsString(map_it)) {
            ESP_LOGE(TAG, "Character in characterMap in not valid");
            characterMap_delete(characterMap);
            return false;
        }
        // TODO: verify if string contains a single UTF-8 character.
        strncpy(&(characterMap->character[i * 4]), map_it->valuestring, 4);
        i++;
    }
    module_setCharacterMap(module, characterMap);
    return true;
}

bool offset_toJson(cJSON **json, module_t *module)
{
    *json = cJSON_CreateNumber(module_getOffset(module));
    return true;
}

bool offset_fromJson(cJSON **json, module_t *module)
{
    GUARD(!(cJSON_IsNumber((*json)) && (*json)->valueint >= 0x00 && (*json)->valueint <= 0xff),
          "property must be a number in range [0-255]");
    module_setOffset(module, (*json)->valueint);
    return true;
}

bool vtrim_toJson(cJSON **json, module_t *module)
{
    *json = cJSON_CreateNumber(module_getVtrim(module));
    return true;
}

bool vtrim_fromJson(cJSON **json, module_t *module)
{
    GUARD(!(cJSON_IsNumber((*json)) && (*json)->valueint >= 0x00 && (*json)->valueint <= 0xff),
          "property must be a number in range [0-255]");
    module_setVtrim(module, (*json)->valueint);
    return true;
}

bool baseSpeed_toJson(cJSON **json, module_t *module)
{
    *json = cJSON_CreateNumber(module_getBaseSpeed(module));
    return true;
}

bool baseSpeed_fromJson(cJSON **json, module_t *module)
{
    GUARD(!(cJSON_IsNumber((*json)) && (*json)->valueint >= 0x00 && (*json)->valueint <= 0xff),
          "property must be a number in range [0-255]");
    module_setBaseSpeed(module, (*json)->valueint);
    return true;
}

void http_moduleEndpointInit()
{
    http_addModulePropertyHandler(PROPERTY_MODULE_INFO, columnEnd_toJson, NULL);
    http_addModulePropertyHandler(PROPERTY_CHARACTER, character_toJson, character_fromJson);
    http_addModulePropertyHandler(characterMapSize_property, characterMapSize_toJson, NULL);
    http_addModulePropertyHandler(PROPERTY_CHARACTER_SET, characterMap_toJson, characterMap_fromJson);
    http_addModulePropertyHandler(PROPERTY_OFFSET, offset_toJson, offset_fromJson);
    http_addModulePropertyHandler(vtrim_property, vtrim_toJson, vtrim_fromJson);
    http_addModulePropertyHandler(baseSpeed_property, baseSpeed_toJson, baseSpeed_fromJson);
}
