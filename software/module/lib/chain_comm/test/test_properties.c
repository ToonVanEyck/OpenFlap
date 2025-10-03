#include "test_properties.h"

cc_prop_t cc_property_list[PROPERTY_CNT] = {
    [PROP_STATIC_RW - 1] =
        {
            .attribute =
                {
                    .read_size  = CC_PROP_SIZE_STATIC(TEST_SIZE),
                    .write_size = CC_PROP_SIZE_STATIC(TEST_SIZE),
                    .name       = "STATIC_RW",
                },
        },
    [PROP_STATIC_RO - 1] =
        {
            .attribute =
                {
                    .read_size  = CC_PROP_SIZE_STATIC(TEST_SIZE),
                    .write_size = CC_PROP_SIZE_NONE,
                    .name       = "STATIC_RO",
                },
        },
    [PROP_STATIC_WO - 1] =
        {
            .attribute =
                {
                    .read_size  = CC_PROP_SIZE_NONE,
                    .write_size = CC_PROP_SIZE_STATIC(TEST_SIZE),
                    .name       = "STATIC_WO",
                },
        },
    [PROP_DYNAMIC_RW - 1] =
        {
            .attribute =
                {
                    .read_size  = CC_PROP_SIZE_DYNAMIC,
                    .write_size = CC_PROP_SIZE_DYNAMIC,
                    .name       = "DYNAMIC_RW",
                },
        },
    [PROP_DYNAMIC_RO - 1] =
        {
            .attribute =
                {
                    .read_size  = CC_PROP_SIZE_DYNAMIC,
                    .write_size = CC_PROP_SIZE_NONE,
                    .name       = "DYNAMIC_RO",
                },
        },
    [PROP_DYNAMIC_WO - 1] =
        {
            .attribute =
                {
                    .read_size  = CC_PROP_SIZE_NONE,
                    .write_size = CC_PROP_SIZE_DYNAMIC,
                    .name       = "DYNAMIC_WO",
                },
        },
};