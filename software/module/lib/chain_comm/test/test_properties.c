#include "test_properties.h"

const cc_prop_attr_t cc_property_attribute_list {
    .[PROP_STATIC_RW] =
        {
            .read_size  = CC_PROP_SIZE_STATIC(TEST_SIZE),
            .write_size = CC_PROP_SIZE_STATIC(TEST_SIZE),
            .name       = "STATIC_RW",
        },
    .[PROP_STATIC_RO] =
        {
            .read_size  = CC_PROP_SIZE_STATIC(TEST_SIZE),
            .write_size = CC_PROP_SIZE_NONE,
            .name       = "STATIC_RO",
        },
    .[PROP_STATIC_WO] =
        {
            .read_size  = CC_PROP_SIZE_NONE,
            .write_size = CC_PROP_SIZE_STATIC(TEST_SIZE),
            .name       = "STATIC_WO",
        },
    .[PROP_DYNAMIC_RW] =
        {
            .read_size  = CC_PROP_SIZE_DYNAMIC,
            .write_size = CC_PROP_SIZE_DYNAMIC,
            .name       = "DYNAMIC_RW",
        },
    .[PROP_DYNAMIC_RO] =
        {
            .read_size  = CC_PROP_SIZE_DYNAMIC,
            .write_size = CC_PROP_SIZE_NONE,
            .name       = "DYNAMIC_RO",
        },
    .[PROP_DYNAMIC_WO] =
        {
            .read_size  = CC_PROP_SIZE_NONE,
            .write_size = CC_PROP_SIZE_DYNAMIC,
            .name       = "DYNAMIC_WO",
        },
};