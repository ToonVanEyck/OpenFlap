#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_log.h"

/* Module types */
#include "module_splitflap.h"

/**
 * \brief Module structure.
 */
typedef struct {
    module_info_property_t module_info; /**< Module info property. */
    command_property_t command;         /**< Command property. */
    union {
        module_splitflap_t splitflap; /**< Splitflap module properties. */
    };
    /** Indicates witch properties need to be synchronized by writing to actual modules. */
    uint64_t sync_properties_write_seq_required;
} module_t;

//---------------------------------------------------------------------------------------------------------------------
