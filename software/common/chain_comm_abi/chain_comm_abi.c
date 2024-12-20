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

const char *chain_comm_command_name_get(command_property_t command)
{
    static const char *command_names[] = {COMMAND_PROPERTY(NAMED_ENUM_NAME)};
    assert(command < CMD_MAX);
    return command_names[command];
}

const char *chain_comm_module_type_name_get(module_type_t module_type)
{
    static const char *module_type_names[] = {MODULE_TYPE_PROPERTY(NAMED_ENUM_NAME)};
    assert(module_type < MODULE_TYPE_MAX);
    return module_type_names[module_type];
}