#include "property_handler.h"
#include <string.h>

extern const property_handler_t PROPERTY_HANDLER_CALIBRATION;
extern const property_handler_t PROPERTY_HANDLER_CHARACTER;
extern const property_handler_t PROPERTY_HANDLER_CHARACTER_SET;
extern const property_handler_t PROPERTY_HANDLER_COMMAND;
extern const property_handler_t PROPERTY_HANDLER_MODULE_INFO;

const property_handler_t *property_handler_get_by_id(property_id_t id)
{
    static const property_handler_t *PROPERTY_HANDLERS[] = {
        &PROPERTY_HANDLER_CALIBRATION, &PROPERTY_HANDLER_CHARACTER,   &PROPERTY_HANDLER_CHARACTER_SET,
        &PROPERTY_HANDLER_COMMAND,     &PROPERTY_HANDLER_MODULE_INFO,
    };

    for (size_t i = 0; i < sizeof(PROPERTY_HANDLERS) / sizeof(PROPERTY_HANDLERS[0]); i++) {
        if (PROPERTY_HANDLERS[i]->id == id) {
            return PROPERTY_HANDLERS[i];
        }
    }
    return NULL; // Not found
}