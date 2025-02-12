#include "property_handler_common.h"

void *property_unique_to_shared(void *unique_property, const module_t *module_list, const size_t module_list_size,
                                property_handler_compare_cb compare)
{
    assert(unique_property != NULL);
    assert(module_list != NULL);
    assert(module_list_size > 0);
    assert(compare != NULL);

    for (size_t i = 0; i < module_list_size; i++) {
        if (compare(unique_property, &module_list[i])) {
            return &module_list[i];
        }
    }

    return NULL;
}