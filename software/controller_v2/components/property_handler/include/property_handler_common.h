#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cJSON.h"
#include "chain_comm_abi.h"
#include "esp_err.h"
#include "esp_log.h"
#include "module.h"

typedef esp_err_t (*property_handler_from_json_handler_cb)(module_t *module, const cJSON *json);
typedef esp_err_t (*property_handler_to_json_handler_cb)(cJSON **json, const module_t *module);
typedef esp_err_t (*property_handler_from_binary_handler_cb)(module_t *module, const uint8_t *bin, uint16_t bin_size);
typedef esp_err_t (*property_handler_to_binary_handler_cb)(uint8_t *bin, uint16_t *bin_size, const module_t *module);

typedef struct {
    property_id_t id; /**< Used as key in binary. */
    property_handler_from_json_handler_cb from_json;
    property_handler_to_json_handler_cb to_json;
    property_handler_from_binary_handler_cb from_binary;
    property_handler_to_binary_handler_cb to_binary;
} property_handler_t;
