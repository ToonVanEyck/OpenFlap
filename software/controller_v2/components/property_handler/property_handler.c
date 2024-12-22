#include "property_handler.h"
#include <string.h>

static const property_handler_t property_handlers[] = {
    PROPERTY_HANDLER_CALIBRATION,   PROPERTY_HANDLER_COMMAND,   PROPERTY_HANDLER_MODULE_INFO,
    PROPERTY_HANDLER_CHARACTER_SET, PROPERTY_HANDLER_CHARACTER,
};

const property_handler_t *property_handler_get_by_id(property_id_t id)
{
    for (uint8_t i = 0; i < sizeof(property_handlers) / sizeof(property_handlers[0]); i++) {
        if (property_handlers[i].id == id) {
            return &property_handlers[i];
        }
    }
    return NULL;
}
