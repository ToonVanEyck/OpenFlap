#include "chain_comm_abi.h"
#include <assert.h>

const char *chain_comm_property_name_get(property_id_t property)
{
    static const char *propertyNames[] = {MODULE_PROPERTY(GENERATE_PROPERTY_NAME)};
    assert(property < PROPERTIES_MAX);
    return propertyNames[property];
}

const chain_comm_binary_attributes_t *chain_comm_property_read_attributes_get(property_id_t property)
{
    static const chain_comm_binary_attributes_t propertyReadAttributes[] = {
        MODULE_PROPERTY(GENERATE_PROPERTY_READ_ATTR)};
    assert(property < PROPERTIES_MAX);
    return &propertyReadAttributes[property];
}

const chain_comm_binary_attributes_t *chain_comm_property_write_attributes_get(property_id_t property)
{
    static const chain_comm_binary_attributes_t propertyWriteAttributes[] = {
        MODULE_PROPERTY(GENERATE_PROPERTY_WRITE_ATTR)};
    assert(property < PROPERTIES_MAX);
    return &propertyWriteAttributes[property];
}

// uint8_t get_property_size(property_id_t property)
// {
//     static const uint8_t propertySizes[] = {MODULE_PROPERTY(GENERATE_PROPERTY_SIZE)};
//     if (property < PROPERTIES_MAX) {
//         return propertySizes[property];
//     }
//     return 0;
// }