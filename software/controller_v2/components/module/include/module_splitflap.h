#pragma once

#include "module_property.h"

/** Calibration properties */
typedef struct {
    uint8_t offset; /**< Offset between the actual character and the index on the encoder wheel. */
} calibration_property_t;

typedef struct {
    calibration_property_t calibration;
} module_splitflap_t;

// static inline void *module_splitflap_property_get_by_id(module_splitflap_t *module, property_id_t property_id)
// {
//     switch (property_id) {
//         case PROPERTY_CALIBRATION:
//             return &module->calibration;
//         case PROPERTY_CHARACTER_SET:
//             return &module->character_set;
//         default:
//             return NULL;
//     }
// }