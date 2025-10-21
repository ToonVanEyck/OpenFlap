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

// const property_handler_t *property_handler_get_by_id(cc_prop_id_t id);

bool prop_handler_from_bin_firmware_update(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size);
bool prop_handler_to_json_firmware_version(void *userdata, uint16_t node_idx, void *data);
bool prop_handler_compare_firmware_version(const void *userdata_a, const void *userdata_b);

static inline void of_property_handlers_init(void)
{
    cc_prop_list[OF_CC_PROP_FIRMWARE_VERSION].handler.set     = prop_handler_from_bin_firmware_update;
    cc_prop_list[OF_CC_PROP_FIRMWARE_VERSION].handler.get     = NULL; /* Not implemented : Read Only */
    cc_prop_list[OF_CC_PROP_FIRMWARE_VERSION].handler.get_alt = prop_handler_to_json_firmware_version;
    cc_prop_list[OF_CC_PROP_FIRMWARE_VERSION].handler.set_alt = NULL; /* Not implemented : Read Only */
    cc_prop_list[OF_CC_PROP_FIRMWARE_VERSION].handler.compare = prop_handler_compare_firmware_version;
}