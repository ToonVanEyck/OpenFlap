#pragma once

#include "properties.h"

typedef struct {
    calibration_property_t calibration;
} module_splitflap_t;

static inline void *module_splitflap_property_get_by_id(module_splitflap_t *module, property_id_t property_id)
{
    switch (property_id) {
        case PROPERTY_CALIBRATION:
            return &module->calibration;
        default:
            return NULL;
    }
}