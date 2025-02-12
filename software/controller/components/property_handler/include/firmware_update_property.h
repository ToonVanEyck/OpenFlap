#pragma once

#include "module.h"

esp_err_t firmware_update_property_set(module_t *module, uint16_t index, const uint8_t *data);
