#include "test_master_properties.h"

void dummy_set_handler(uint16_t node_idx, uint8_t *buf, uint16_t *size, void *userdata)
{
    printf("Dummy set handler called for node %d with size %d and user data %p:\n", node_idx, *size, userdata);
    for (uint16_t i = 0; i < *size; i++) {
        printf("  Data[%d]: %02X\n", i, buf[i]);
    }
}

void dummy_get_handler(uint16_t node_idx, uint8_t *buf, uint16_t *size, void *userdata)
{
    *size = TEST_SIZE;
    printf("Dummy get handler called for node %d with size %d and user data %p\n", node_idx, *size, userdata);
    for (uint16_t i = 0; i < *size; i++) {
        buf[i] = (uint8_t)(i);
    }
}

const cc_prop_handler_t master_property_handlers[PROPERTY_CNT] = {
    [PROP_STATIC_RW]  = {.get = dummy_get_handler, .set = dummy_set_handler},
    [PROP_STATIC_RO]  = {.get = NULL, .set = dummy_set_handler},
    [PROP_STATIC_WO]  = {.get = dummy_get_handler, .set = NULL},
    [PROP_DYNAMIC_RW] = {.get = dummy_get_handler, .set = dummy_set_handler},
    [PROP_DYNAMIC_RO] = {.get = NULL, .set = dummy_set_handler},
    [PROP_DYNAMIC_WO] = {.get = dummy_get_handler, .set = NULL},
};