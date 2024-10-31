#include "properties.h"
#include <string.h>

static const property_handler_t properties[] = {
    PROPERTY_HANDLER_CALIBRATION,
    PROPERTY_HANDLER_COMMAND,
    PROPERTY_HANDLER_MODULE_INFO,
};

const property_handler_t *property_handler_get_by_id(property_id_t id)
{
    for (uint8_t i = 0; i < sizeof(properties) / sizeof(properties[0]); i++) {
        if (properties[i].id == id) {
            return &properties[i];
        }
    }
    return NULL;
}

const property_handler_t *property_handler_get_by_name(const char *name)
{
    for (uint8_t i = 0; i < sizeof(properties) / sizeof(properties[0]); i++) {
        if (strcmp(properties[i].name, name) == 0) {
            return &properties[i];
        }
    }
    return NULL;
}

const char *property_name_get_by_id(property_id_t id)
{
    const property_handler_t *property = property_handler_get_by_id(id);
    if (property == NULL) {
        return NULL;
    }
    return property->name;
}