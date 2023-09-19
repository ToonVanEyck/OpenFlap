#include "UartApi.h"

static const char *TAG = "[UART_API]";

void columnEnd_deserialize(char *data, module_t *module)
{
    module_setColumnEnd(module, data[0]);
}

void character_serialize(char *data, module_t *module)
{
    data[0] = module_getCharacterIndex(module);
}

void character_deserialize(char *data, module_t *module)
{
    module_setCharacterIndex(module, data[0]);
}

void characterMapSize_deserialize(char *data, module_t *module)
{
    uint8_t characterMapSize = data[0];
    characterMap_t *characterMap = characterMap_new(characterMapSize);
    // memcpy(characterMap->character,data,characterMapSize*4);
    module_setCharacterMap(module, characterMap);
}

void characterMap_serialize(char *data, module_t *module)
{
    uint8_t characterMapSize = module_getCharacterMapSize(module);
    characterMap_t *characterMap = module_getCharacterMap(module);
    memcpy(data, characterMap->character, characterMapSize * 4);
}

void characterMap_deserialize(char *data, module_t *module)
{
    uint8_t characterMapSize = 48;
    characterMap_t *characterMap = characterMap_new(characterMapSize);
    memcpy(characterMap->character, data, characterMapSize * 4);
    module_setCharacterMap(module, characterMap);
}

void offset_serialize(char *data, module_t *module)
{
    data[0] = module_getOffset(module);
}

void offset_deserialize(char *data, module_t *module)
{
    module_setOffset(module, data[0]);
}

void vtrim_serialize(char *data, module_t *module)
{
    data[0] = module_getVtrim(module);
}

void vtrim_deserialize(char *data, module_t *module)
{
    module_setVtrim(module, data[0]);
}

void baseSpeed_serialize(char *data, module_t *module)
{
    data[0] = module_getBaseSpeed(module);
}

void baseSpeed_deserialize(char *data, module_t *module)
{
    module_setBaseSpeed(module, data[0]);
}

void uart_api_init()
{
    uart_addModulePropertyHandler(columnEnd_property, columnEnd_deserialize, NULL);
    uart_addModulePropertyHandler(character_property, character_deserialize, character_serialize);
    uart_addModulePropertyHandler(characterMapSize_property, characterMapSize_deserialize, NULL);
    uart_addModulePropertyHandler(characterMap_property, characterMap_deserialize, characterMap_serialize);
    uart_addModulePropertyHandler(offset_property, offset_deserialize, offset_serialize);
    uart_addModulePropertyHandler(vtrim_property, vtrim_deserialize, vtrim_serialize);
    uart_addModulePropertyHandler(baseSpeed_property, baseSpeed_deserialize, baseSpeed_serialize);
}