#include "test_properties.h"

cc_prop_t cc_prop_list[PROPERTY_CNT] = {
    [PROP_STATIC_RW] =
        {
            .attribute =
                {
                    // .read_size  = CC_PROP_SIZE_STATIC(TEST_PROP_SIZE),
                    // .write_size = CC_PROP_SIZE_STATIC(TEST_PROP_SIZE),
                    .name = "STATIC_RW",
                },
        },
    [PROP_STATIC_RO] =
        {
            .attribute =
                {
                    // .read_size  = CC_PROP_SIZE_STATIC(TEST_PROP_SIZE),
                    // .write_size = CC_PROP_SIZE_NONE,
                    .name = "STATIC_RO",
                },
        },
    [PROP_STATIC_WO] =
        {
            .attribute =
                {
                    // .read_size  = CC_PROP_SIZE_NONE,
                    // .write_size = CC_PROP_SIZE_STATIC(TEST_PROP_SIZE),
                    .name = "STATIC_WO",
                },
        },
    [PROP_DYNAMIC_RW] =
        {
            .attribute =
                {
                    // .read_size  = CC_PROP_SIZE_DYNAMIC,
                    // .write_size = CC_PROP_SIZE_DYNAMIC,
                    .name = "DYNAMIC_RW",
                },
        },
    [PROP_DYNAMIC_RO] =
        {
            .attribute =
                {
                    // .read_size  = CC_PROP_SIZE_DYNAMIC,
                    // .write_size = CC_PROP_SIZE_NONE,
                    .name = "DYNAMIC_RO",
                },
        },
    [PROP_DYNAMIC_WO] =
        {
            .attribute =
                {
                    // .read_size  = CC_PROP_SIZE_NONE,
                    // .write_size = CC_PROP_SIZE_DYNAMIC,
                    .name = "DYNAMIC_WO",
                },
        },
    [PROP_STATIC_RW_HALF_SIZE] =
        {
            .attribute =
                {
                    // .read_size  = CC_PROP_SIZE_STATIC(TEST_PROP_SIZE / 2),
                    // .write_size = CC_PROP_SIZE_STATIC(TEST_PROP_SIZE / 2),
                    .name = "STATIC_RW_HALF_SIZE",
                },
        },
};