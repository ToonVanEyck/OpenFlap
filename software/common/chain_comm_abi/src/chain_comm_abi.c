#include "chain_comm_abi.h"

const char *get_property_name(module_property_t property)
{
#ifdef GENERATE_PROPERTY_NAMES
    static const char *propertyNames[] = {MODULE_PROPERTY(GENERATE_PROPERTY_NAME)};
    if (property < end_of_properties) {
        return propertyNames[property];
    }
#endif
    return "undefined";
}

const uint8_t get_property_size(module_property_t property)
{
    static const uint8_t propertySizes[] = {MODULE_PROPERTY(GENERATE_PROPERTY_SIZE)};
    if (property < end_of_properties) {
        return propertySizes[property];
    }
    return 0;
}
