#pragma once

#include "module_property.h"

/** Calibration properties */
typedef struct {
    uint8_t offset; /**< Offset between the actual character and the index on the encoder wheel. */
} calibration_property_t;

typedef struct {
    calibration_property_t calibration;
} module_splitflap_t;
