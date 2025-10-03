#include "test_node_properties.h"
#include "test_properties.h"

#include <stdio.h>

void node_set_handler(uint16_t node_idx, uint8_t *buf, uint16_t *size, void *userdata)
{
    printf("Dummy set handler called for node %d with size %d and user data %p:\n", node_idx, *size, userdata);
    for (uint16_t i = 0; i < *size; i++) {
        printf("  Data[%d]: %02X\n", i, buf[i]);
    }
}

void node_get_handler(uint16_t node_idx, uint8_t *buf, uint16_t *size, void *userdata)
{
    *size = TEST_SIZE;
    printf("Dummy get handler called for node %d with size %d and user data %p\n", node_idx, *size, userdata);
    for (uint16_t i = 0; i < *size; i++) {
        buf[i] = (uint8_t)(i);
    }
}

void setup_cc_node_property_list_handlers(void)
{
    cc_property_list[PROP_STATIC_RW - 1].handler.get = node_get_handler;
    cc_property_list[PROP_STATIC_RW - 1].handler.set = node_set_handler;

    cc_property_list[PROP_STATIC_RO - 1].handler.get = NULL;
    cc_property_list[PROP_STATIC_RO - 1].handler.set = node_set_handler;

    cc_property_list[PROP_STATIC_WO - 1].handler.get = node_get_handler;
    cc_property_list[PROP_STATIC_WO - 1].handler.set = NULL;

    cc_property_list[PROP_DYNAMIC_RW - 1].handler.get = node_get_handler;
    cc_property_list[PROP_DYNAMIC_RW - 1].handler.set = node_set_handler;

    cc_property_list[PROP_DYNAMIC_RO - 1].handler.get = NULL;
    cc_property_list[PROP_DYNAMIC_RO - 1].handler.set = node_set_handler;

    cc_property_list[PROP_DYNAMIC_WO - 1].handler.get = node_get_handler;
    cc_property_list[PROP_DYNAMIC_WO - 1].handler.set = NULL;
}