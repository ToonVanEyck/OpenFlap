#ifndef MODEL_H
#define MODEL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"

#include "chain_comm_abi.h"

#include "board_io.h"

typedef struct {
    uint8_t offset;
    uint8_t vtrim;
} Calibration_t;

typedef struct {
    uint8_t size;
    char *character; // UTF-8
    int reffCnt;
} characterMap_t;

typedef struct {
    uint8_t characterIndex; // index of the current character in the characterMap
    Calibration_t calibration;
    characterMap_t *characterMap;
    uint8_t baseSpeed;
    char *firmwareVersion;
    bool colEnd;
    uint64_t updatableProperties;
} module_t;

typedef struct {
    // size_t width;
    // size_t height;
    size_t size;
    module_t *module;
    // Transition_t transition;
    uint64_t requestedProperties;
} Display_t;

typedef struct {
    Display_t display;
    bool modulesPowered;
    char *firmwareVersion;
} controller_t;

typedef struct {
    TaskHandle_t task;
    controller_t *controller;
} OpenFlapModel_ctx_t;

typedef enum {
    fromNothing,
    fromHttp,
    fromUart,
    fromFirmware,
} modelNotificationSrc_t;

TaskHandle_t modelTask();

void flap_model_init();
void model_preformUart();

controller_t *controller_new();
void controller_delete(controller_t *controller);

void display_requestModuleProperty(moduleProperty_t property);
uint64_t display_getRequestModuleProperties();

void display_setPowered(bool powered);
bool display_getPowered();
module_t *display_getModule(size_t i);
// bool display_setDimensions(size_t width, size_t height);
bool display_setSize(size_t size);
bool display_setMessage(Display_t *display, char *message);
size_t display_getWidth();
size_t display_getHeight();
size_t display_getSize();

characterMap_t *characterMap_new();
void characterMap_delete(characterMap_t *characterMap);
void characterMap_getCharacter(characterMap_t *characterMap, size_t index,
                               char *character); // character must be an array of 5 bytes. This array will be populated
                                                 // with a null terminated UTF-8 encoded character.
bool characterMap_isEqual(characterMap_t *a, characterMap_t *b);

void module_delete(module_t *module);

bool module_getColumnEnd(module_t *module);
void module_setColumnEnd(module_t *module, bool colEnd);

char *module_getCharacter(module_t *module);
void module_setCharacter(module_t *module, char *character);
void module_setCharacterIndex(module_t *module, uint8_t characterIndex);
uint8_t module_getCharacterIndex(module_t *module);

uint8_t module_getCharacterMapSize(module_t *module);

characterMap_t *module_getCharacterMap(module_t *module);
void module_setCharacterMap(module_t *module, characterMap_t *characterMap);

uint8_t module_getOffset(module_t *module);
void module_setOffset(module_t *module, uint8_t offset);

uint8_t module_getVtrim(module_t *module);
void module_setVtrim(module_t *module, uint8_t vtrim);

uint8_t module_getBaseSpeed(module_t *module);
void module_setBaseSpeed(module_t *module, uint8_t baseSpeed);
#endif