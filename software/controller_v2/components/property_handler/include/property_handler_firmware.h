#pragma once

#include "esp_err.h"
#include "module.h"

esp_err_t property_handler_firmware_set(module_t *module, uint16_t index, const uint8_t *data, uint16_t data_size);