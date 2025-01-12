#include "property_handler.h"
#include <string.h>

/* Declare an external symbol for the handlers array. */
// extern const property_handler_t _property_handlers_start[];
// extern const property_handler_t _property_handlers_end[];

const property_handler_t *property_handler_get_by_id(property_id_t id)
{
    // for (const property_handler_t *handler = _property_handlers_start; handler < _property_handlers_end; ++handler) {
    //     if (handler->id == id) {
    //         return handler;
    //     }
    // }
    return NULL; // Not found
}