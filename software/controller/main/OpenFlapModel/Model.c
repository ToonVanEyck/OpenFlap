
#define DO_GENERATE_PROPERTY_NAMES
#include "Model.h"
#include "flap_http_server.h"
#include "flap_uart.h"

static const char *TAG = "[MODEL]";

static OpenFlapModel_ctx_t ctx;

static void OpenFlapModelTask(void *arg)
{
    while (1) {
        if (ulTaskNotifyTake(true, 250 / portTICK_RATE_MS) == fromHttp) {

            // if (ctx.controller->display.requestedProperties) {
            //     // send command to switch from bootloader to application
            //     msg_newWriteAll(PROPERTY_COMMAND);
            //     msg_addData(runApp_command);
            //     msg_addData(PROPERTY_NONE);
            //     msg_send(MAX_COMMAND_PERIOD_MS);
            //     ulTaskNotifyTake(true, 5000 / portTICK_RATE_MS); // wait for command to finish
            // }

            ESP_LOGI(TAG, "Requested properties: 0x%08llx", ctx.controller->display.requestedProperties);
            for (property_id_t property = PROPERTY_NONE + 1; property < PROPERTIES_MAX; property++) {
                if (ctx.controller->display.requestedProperties & (1 << property)) {
                    ctx.controller->display.requestedProperties ^= (1 << property);
                    ESP_LOGI(TAG, "read property: %d %s", property, chain_comm_property_name_get(property));
                    uart_propertyReadAll(property);
                }
            }

            // handle write updates
            uint64_t updatableProperties                = PROPERTY_NONE;
            uint64_t updatablePropertiesWriteAll        = PROPERTY_NONE;
            uint64_t updatablePropertiesWriteSequential = PROPERTY_NONE;
            for (int i = 0; i < display_getSize(); i++) {
                module_t *module = display_getModule(i);
                updatableProperties |= module->updatableProperties;
            }
            for (property_id_t property = PROPERTY_NONE + 1; property < PROPERTIES_MAX; property++) {
                updatablePropertiesWriteAll |=
                    ((updatableProperties & (1 << property) && uart_moduleSerializedPropertiesAreEqual(property))
                     << property);
            }
            updatablePropertiesWriteSequential = updatableProperties ^ updatablePropertiesWriteAll;

            ESP_LOGI(TAG, "Updatable properties with WriteAll command: 0x%04llX", updatablePropertiesWriteAll);
            ESP_LOGI(TAG, "Updatable properties with WriteSequential command: 0x%04llX",
                     updatablePropertiesWriteSequential);
            for (property_id_t property = PROPERTY_NONE + 1; property < PROPERTIES_MAX; property++) {
                if (updatablePropertiesWriteAll & (1 << property)) {
                    ESP_LOGI(TAG, "updateing property: %d", property);
                    uart_propertyWriteAll(property);
                } else if (updatablePropertiesWriteSequential & (1 << property)) {
                    ESP_LOGI(TAG, "updateing property: %d", property);
                    uart_propertyWriteSequential(property);
                }
            }
            // finished all model updates
            xTaskNotify(httpTask(), 1, eSetValueWithoutOverwrite);
        }
    }
}

TaskHandle_t modelTask()
{
    return ctx.task;
}

void flap_model_init(void)
{
    ctx.controller                              = controller_new();
    ctx.controller->display.requestedProperties = 0;
    xTaskCreate(OpenFlapModelTask, "OpenFlap Model task", 6000, NULL, 10, &ctx.task);
}

void model_preformUart()
{
    xTaskNotify(modelTask(), fromHttp, eSetValueWithoutOverwrite);
    ulTaskNotifyTake(true, 10000 / portTICK_RATE_MS);
}

// Controller

controller_t *controller_new()
{
    controller_t *controller = malloc(sizeof(controller_t));
    if (controller == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for Controller object");
        return NULL;
    }
    controller->display.size    = 0;
    controller->display.module  = NULL;
    controller->modulesPowered  = false;       // getRelayState
    controller->firmwareVersion = __VERSION__; // getFwVersion
    return controller;
}

void controller_delete(controller_t *controller)
{
    // Free all modules
    if (controller->display.module) {
        for (int i = 0; i < controller->display.size; i++) {
            module_delete(&(controller->display.module[i]));
        }
    }
}

// Display

void display_requestModuleProperty(property_id_t property)
{
    ctx.controller->display.requestedProperties |= (1 << property);
}

uint64_t display_getRequestModuleProperties()
{
    return ctx.controller->display.requestedProperties;
}

bool display_getPowered()
{
    return gpio_get_level(FLAP_ENABLE_PIN);
}
void display_setPowered(bool powered)
{
    gpio_set_level(FLAP_ENABLE_PIN, powered);
    vTaskDelay(50 / portTICK_PERIOD_MS);
}

module_t *display_getModule(size_t index)
{
    if (index >= display_getSize()) {
        ESP_LOGE(TAG, "Module index %i is out of range.", index);
        return NULL;
    }
    return ctx.controller->display.module + index;
}

bool display_setSize(size_t size)
{
    Display_t *display = &ctx.controller->display;
    if (display->size == size) {
        return true;
    }
    size_t oldSize = display->size;
    display->size  = size;
    ESP_LOGI(TAG, "Changing display size from %d to %d", oldSize, display->size);
    display->module = realloc(display->module, display->size * sizeof(module_t));
    if (display->module == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for new display size.");
        return false;
    }
    int newMem = (long)display->size - (long)oldSize;
    if (newMem > 0) {
        memset(&(display->module[oldSize]), 0, newMem * sizeof(module_t));
    }
    return true;
}

bool display_setMessage(Display_t *display, char *message)
{
    return true;
}

size_t display_getWidth()
{
    return ctx.controller->display.size;
}

size_t display_getHeight()
{
    return ctx.controller->display.size;
}

size_t display_getSize()
{
    return ctx.controller->display.size;
}

// Charsets

characterMap_t *characterMap_new(size_t size)
{
    characterMap_t *characterMap = malloc(sizeof(characterMap_t));
    if (characterMap == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for Charset object");
        return NULL;
    }
    characterMap->size      = size;
    characterMap->reffCnt   = 1;
    characterMap->character = calloc(size, 4 * sizeof(char));
    if (characterMap->character == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for Charset characters");
        free(characterMap);
        return NULL;
    }
    return characterMap;
}

void characterMap_delete(characterMap_t *characterMap)
{
    if (characterMap) {
        characterMap->reffCnt--;
        if (!characterMap->reffCnt) {
            if (characterMap->character) {
                free(characterMap->character);
            }
            free(characterMap);
        }
    }
}

void characterMap_getCharacter(characterMap_t *characterMap, size_t index, char *character)
{
    memcpy(character, &characterMap->character[index * 4], 4);
    character[5] = 0;
}

bool characterMap_isEqual(characterMap_t *a, characterMap_t *b)
{
    if (!a || !b || a->size != b->size) {
        return false;
    }
    for (int i = 0; i < a->size * 4; i++) {
        if (a->character[i] != b->character[i]) {
            return false;
        }
    }
    return true;
}

// Modules

void module_delete(module_t *module)
{
    if (module) {
        if (module->firmwareVersion) {
            free(module->firmwareVersion);
        }
        characterMap_delete(module->characterMap);
        free(module);
    }
}

bool module_getColumnEnd(module_t *module)
{
    return module->colEnd;
}
void module_setColumnEnd(module_t *module, bool colEnd)
{
    module->colEnd = colEnd;
}

char *module_getCharacter(module_t *module)
{
    if (!module->characterMap || !module->characterMap->size) {
        ESP_LOGE(TAG, "Module does not have a valid characterMap");
        return NULL;
    }
    if (module->characterMap->size < module->characterIndex) {
        ESP_LOGE(TAG, "Module character not in range of characterMap");
        return NULL;
    }
    return module->characterMap->character + 4 * module_getCharacterIndex(module);
}
void module_setCharacter(module_t *module, char *character)
{
    char buf[4] = {0};
    strncpy(buf, character, 4);
    // check if the char is in the characterMap.
    for (int i = 0; i < module->characterMap->size; i++) {
        if (module->characterMap->character[4 * i + 0] == buf[0] &&
            module->characterMap->character[4 * i + 1] == buf[1] &&
            module->characterMap->character[4 * i + 2] == buf[2] &&
            module->characterMap->character[4 * i + 3] == buf[3]) {
            module_setCharacterIndex(module, i);
            return;
        }
    }
}

void module_setCharacterIndex(module_t *module, uint8_t characterIndex)
{
    module->characterIndex = characterIndex;
    module->updatableProperties |= (1 << PROPERTY_CHARACTER);
}

uint8_t module_getCharacterIndex(module_t *module)
{
    return module->characterIndex;
}

uint8_t module_getCharacterMapSize(module_t *module)
{

    return module->characterMap ? module->characterMap->size : 0;
}

characterMap_t *module_getCharacterMap(module_t *module)
{
    return module->characterMap;
}

void module_setCharacterMap(module_t *module, characterMap_t *characterMap)
{
    for (int i = 0; i < display_getSize(); i++) {
        if (characterMap_isEqual(display_getModule(i)->characterMap, characterMap)) {
            characterMap_delete(characterMap);                 // delete "new" characterMap
            characterMap = display_getModule(i)->characterMap; // refer to existing characterMap
            characterMap->reffCnt++;
            break;
        }
    }
    characterMap_delete(module->characterMap);
    module->characterMap = characterMap;
    module->updatableProperties |= (1 << PROPERTY_CHARACTER_SET);
}

uint8_t module_getOffset(module_t *module)
{
    return module->calibration.offset;
}
void module_setOffset(module_t *module, uint8_t offset)
{
    module->calibration.offset = offset;
    module->updatableProperties |= (1 << PROPERTY_CALIBRATION);
}

uint8_t module_getVtrim(module_t *module)
{
    return module->calibration.vtrim;
}
void module_setVtrim(module_t *module, uint8_t vtrim)
{
    module->calibration.vtrim = vtrim;
    module->updatableProperties |= (1 << vtrim_property);
}

uint8_t module_getBaseSpeed(module_t *module)
{
    return module->baseSpeed;
}
void module_setBaseSpeed(module_t *module, uint8_t baseSpeed)
{
    module->baseSpeed = baseSpeed;
    module->updatableProperties |= (1 << baseSpeed_property);
}