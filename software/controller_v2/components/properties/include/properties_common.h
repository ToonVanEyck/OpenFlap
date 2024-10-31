#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cJSON.h"
#include "esp_log.h"

/** List of all available properties. */
typedef enum {
    PROPERTY_NONE,
    PROPERTY_FIRMWARE,
    PROPERTY_COMMAND,
    PROPERTY_MODULE_INFO,
    PROPERTY_MOTION_PARAMS,
    PROPERTY_CHARACTER_SET,
    PROPERTY_CHARACTER,
    PROPERTY_VISUAL_PARAMS,

    PROPERTY_COLUMN_END,
    PROPERTY_CALIBRATION, /**< See calibration_property.h */
    PROPERTY_SPEED_CURVE,
    PROPERTIES_MAX,
} property_id_t;

typedef bool (*property_handler_from_json_handler_cb)(void *property, const cJSON *json);
typedef bool (*property_handler_to_json_handler_cb)(cJSON **json, const void *property);
typedef bool (*property_handler_from_binary_handler_cb)(void *property, const uint8_t *bin, uint8_t index);
typedef bool (*property_handler_to_binary_handler_cb)(uint8_t *bin, const void *property, uint8_t index);

typedef struct {
    bool multipart;
    bool dynamic_size;
    uint8_t static_size;
} property_handler_binary_attributes_t;

typedef struct {
    property_id_t id; /**< Used as key in binary. */
    const char *name; /**< Used as key in json. */
    property_handler_from_json_handler_cb from_json;
    property_handler_to_json_handler_cb to_json;
    property_handler_from_binary_handler_cb from_binary;
    property_handler_to_binary_handler_cb to_binary;
    property_handler_binary_attributes_t from_binary_attributes;
    property_handler_binary_attributes_t to_binary_attributes;
} property_handler_t;