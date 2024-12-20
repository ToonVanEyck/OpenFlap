#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cJSON.h"
#include "chain_comm_abi.h"
#include "esp_err.h"
#include "esp_log.h"

typedef esp_err_t (*property_handler_from_json_handler_cb)(void *property, const cJSON *json);
typedef esp_err_t (*property_handler_to_json_handler_cb)(cJSON **json, const void *property);
typedef esp_err_t (*property_handler_from_binary_handler_cb)(void *property, const uint8_t *bin, uint8_t index);
typedef esp_err_t (*property_handler_to_binary_handler_cb)(uint8_t *bin, const void *property, uint8_t index);

typedef struct {
    property_id_t id; /**< Used as key in binary. */
    property_handler_from_json_handler_cb from_json;
    property_handler_to_json_handler_cb to_json;
    property_handler_from_binary_handler_cb from_binary;
    property_handler_to_binary_handler_cb to_binary;
} property_handler_t;
