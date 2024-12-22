#pragma once

#include "property_handlers/calibration_property.h"
#include "property_handlers/character_property.h"
#include "property_handlers/character_set_property.h"
#include "property_handlers/command_property.h"
#include "property_handlers/module_info_property.h"

const property_handler_t *property_handler_get_by_id(property_id_t id);
