#pragma once

#include "calibration_property.h"
#include "command_property.h"
#include "module_info_property.h"

const property_handler_t *property_handler_get_by_id(property_id_t id);
const property_handler_t *property_handler_get_by_name(const char *name);
const char *property_name_get_by_id(property_id_t id);