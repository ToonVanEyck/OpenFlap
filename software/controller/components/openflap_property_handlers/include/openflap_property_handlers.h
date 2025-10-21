#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cJSON.h"
#include "esp_err.h"
#include "esp_log.h"
#include "openflap_display.h"
#include "openflap_module.h"
#include "openflap_properties.h"

/**
 * @brief Initialize the property handlers.
 */
void of_property_handlers_init(void);
