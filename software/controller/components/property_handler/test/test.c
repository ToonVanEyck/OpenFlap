#include "chain_comm_abi.h"
#include "esp_err.h"
#include "esp_system.h"
#include "memory_checks.h"
#include "module.h"
#include "property_handler.h"
#include "unity.h"
#include <string.h>

TEST_CASE("Test property_handler_get_by_id: ok", "[properties][qemu]")
{
    const property_handler_t *property = property_handler_get_by_id(PROPERTY_OFFSET);
    TEST_ASSERT_NOT_NULL(property);
    TEST_ASSERT_EQUAL_STRING("calibration", chain_comm_property_name_by_id(property->id));
}

TEST_CASE("Test property_handler_get_by_id: id not found", "[properties][qemu]")
{
    const property_handler_t *property = property_handler_get_by_id(PROPERTIES_MAX);
    TEST_ASSERT_NULL(property);
}

#define SYMBOL_€ (0x00ac82e2) // € symbol
TEST_CASE("Test to_binary handler", "[properties][qemu]")
{
    const property_handler_t *property = property_handler_get_by_id(PROPERTY_CHARACTER_SET);
    TEST_ASSERT_NOT_NULL(property);

    module_t module = {
        .module_info     = {.type = MODULE_TYPE_SPLITFLAP},
        .character_index = 0,
        .character_set   = character_set_new(48),
    };
    memcpy(module.character_set->data,
           (uint32_t[]) {
               (uint32_t)' ', (uint32_t)'A', (uint32_t)'B', (uint32_t)'C', (uint32_t)'D', (uint32_t)'E', (uint32_t)'F',
               (uint32_t)'G', (uint32_t)'H', (uint32_t)'I', (uint32_t)'J', (uint32_t)'K', (uint32_t)'L', (uint32_t)'M',
               (uint32_t)'N', (uint32_t)'O', (uint32_t)'P', (uint32_t)'Q', (uint32_t)'R', (uint32_t)'S', (uint32_t)'T',
               (uint32_t)'U', (uint32_t)'V', (uint32_t)'W', (uint32_t)'X', (uint32_t)'Y', (uint32_t)'Z', (uint32_t)'0',
               (uint32_t)'1', (uint32_t)'2', (uint32_t)'3', (uint32_t)'4', (uint32_t)'5', (uint32_t)'6', (uint32_t)'7',
               (uint32_t)'8', (uint32_t)'9', SYMBOL_€,      (uint32_t)'$', (uint32_t)'!', (uint32_t)'?', (uint32_t)'.',
               (uint32_t)',', (uint32_t)':', (uint32_t)'/', (uint32_t)'@', (uint32_t)'#', (uint32_t)'&',
           },
           48 * 4);

    uint8_t *property_data = NULL;
    uint16_t property_size = 0;

    property->to_binary(&property_data, &property_size, &module);
    free(property_data);
}

TEST_CASE("Test json_to_color handler", "[properties][qemu]")
{
    const property_handler_t *property = property_handler_get_by_id(PROPERTY_COLOR);
    TEST_ASSERT_NOT_NULL(property);

    cJSON *json     = cJSON_Parse("{\"foreground\":\"#ff00ff\",\"background\":\"#00ff00\"}");
    module_t module = {0};

    property->from_json(&module, json);
}