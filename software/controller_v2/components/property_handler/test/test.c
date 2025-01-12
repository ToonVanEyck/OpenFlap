#include "chain_comm_abi.h"
#include "esp_err.h"
#include "esp_system.h"
#include "memory_checks.h"
#include "module.h"
#include "property_handler.h"
#include "unity.h"

TEST_CASE("Test property_handler_get_by_id: ok", "[properties][qemu]")
{
    const property_handler_t *property = property_handler_get_by_id(PROPERTY_CALIBRATION);
    TEST_ASSERT_NOT_NULL(property);
    TEST_ASSERT_EQUAL_STRING("calibration", chain_comm_property_name_by_id(property->id));
}

TEST_CASE("Test property_handler_get_by_id: id not found", "[properties][qemu]")
{
    const property_handler_t *property = property_handler_get_by_id(PROPERTIES_MAX);
    TEST_ASSERT_NULL(property);
}

TEST_CASE("Test to_binary handler", "[properties][qemu]")
{
    const property_handler_t *property = property_handler_get_by_id(PROPERTY_CHARACTER_SET);
    TEST_ASSERT_NOT_NULL(property);

    module_t module = {
        .module_info     = {.type = MODULE_TYPE_SPLITFLAP},
        .character_index = 0,
        .character_set =
            {
                .size          = 48,
                .character_set = (uint8_t *)((char *) {
                    "A\0\0\0B\0\0\0€\0A\0\0\0B\0\0\0€\0A\0\0\0B\0\0\0€\0A\0\0\0B\0\0\0€\0A\0\0\0B\0\0\0€\0A\0\0\0B\0\0"
                    "\0€\0A\0\0\0B\0\0\0€\0A\0\0\0B\0\0\0€\0A\0\0\0B\0\0\0€\0A\0\0\0B\0\0\0€\0A\0\0\0B\0\0\0€\0A\0\0\0B"
                    "\0\0\0€\0A\0\0\0B\0\0\0€\0A\0\0\0B\0\0\0€\0A\0\0\0B\0\0\0€\0A\0\0\0B\0\0\0€\0"}),
            },
    };

    uint8_t *property_data = NULL;
    uint16_t property_size = 0;

    property->to_binary(&property_data, &property_size, &module);
    free(property_data);
}