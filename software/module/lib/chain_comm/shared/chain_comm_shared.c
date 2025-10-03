#include "chain_comm_shared.h"
#include <assert.h>
#include <string.h>

// const char *cc_property_name_by_id(cc_prop_id_t property)
// {
//     static const char *propertyNames[] = {MODULE_PROPERTY(GENERATE_PROPERTY_NAME)};
//     assert(property < PROPERTIES_MAX);
//     return propertyNames[property];
// }

// cc_prop_id_t cc_property_id_by_name(const char *name)
// {
//     for (cc_prop_id_t i = PROPERTY_NONE + 1; i < PROPERTIES_MAX; i++) {
//         if (strcmp(name, cc_property_name_by_id(i)) == 0) {
//             return i;
//         }
//     }
//     return PROPERTY_NONE;
// }

// const cc_property_handler_t *cc_property_handler_by_id(cc_prop_id_t property)
// {
//     static const cc_property_handler_t propertyHandlers[] = {MODULE_PROPERTY(GENERATE_PROPERTY_NAME)};
//     assert(property < PROPERTIES_MAX);
//     return &propertyHandlers[property];
// }

// const cc_prop_attr_t *cc_property_read_attributes_get(cc_prop_id_t property)
// {
//     static const cc_prop_attr_t propertyReadAttributes[] = {MODULE_PROPERTY(GENERATE_PROPERTY_READ_ATTR)};
//     assert(property < PROPERTIES_MAX);
//     return &propertyReadAttributes[property];
// }

// const cc_prop_attr_t *cc_property_write_attributes_get(cc_prop_id_t property)
// {
//     static const cc_prop_attr_t propertyWriteAttributes[] = {MODULE_PROPERTY(GENERATE_PROPERTY_WRITE_ATTR)};
//     assert(property < PROPERTIES_MAX);
//     return &propertyWriteAttributes[property];
// }
