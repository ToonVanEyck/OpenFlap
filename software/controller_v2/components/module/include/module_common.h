#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "chain_comm_abi.h"
#include "esp_log.h"

/* Module types */
#include "module_splitflap.h"

/** Character Set properties */
typedef struct {
    uint8_t size; /**< Size of the character set ( 1/4 of character_set array. ) */
    /** List of supported characters, 4 bytes will be allocated per character so all UTF-8 characters are supported.  */
    uint8_t *character_set;
} character_set_property_t;

/**
 * \brief Module structure.
 */
typedef struct {
    module_info_property_t module_info; /**< Module info property. */
    command_property_t command;         /**< Command property. */
    uint8_t character_index; /**< The index of the character in the charactermap that is currently being displayed. */
    character_set_property_t character_set;
    union {
        module_splitflap_t splitflap; /**< Splitflap module properties. */
    };
    /** Indicates witch properties need to be synchronized by writing to actual modules. */
    uint64_t sync_properties_write_seq_required;
} module_t;

//---------------------------------------------------------------------------------------------------------------------
