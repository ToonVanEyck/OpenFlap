#include "calibration_property.h"
#include "chain_comm_abi.h"
#include "esp_err.h"
#include "esp_system.h"
#include "memory_checks.h"
#include "properties.h"
#include "unity.h"

TEST_CASE("Test Get Properties by id", "[properties][qemu]")
{
    const property_handler_t *property = property_handler_get_by_id(PROPERTY_CALIBRATION);
    TEST_ASSERT_NOT_NULL(property);
    TEST_ASSERT_EQUAL_STRING("calibration", chain_comm_property_name_get(property->id));
}

TEST_CASE("Test Get Properties by id not found", "[properties][qemu]")
{
    const property_handler_t *property = property_handler_get_by_id(PROPERTIES_MAX);
    TEST_ASSERT_NULL(property);
}

TEST_CASE("Test Get Properties by name", "[properties][qemu]")
{
    const property_handler_t *property =
        property_handler_get_by_name(chain_comm_property_name_get(PROPERTY_HANDLER_CALIBRATION.id));
    TEST_ASSERT_NOT_NULL(property);
    TEST_ASSERT_EQUAL(PROPERTY_CALIBRATION, property->id);
}

TEST_CASE("Test Get Properties by name not found", "[properties][qemu]")
{
    const property_handler_t *property = property_handler_get_by_name("not_found");
    TEST_ASSERT_NULL(property);
}

TEST_CASE("Test Property From JSON", "[properties][qemu]")
{
    cJSON *json = cJSON_Parse("{\"offset\": 1, \"vtrim\": 2}");
    calibration_property_t calibration;
    const property_handler_t *property =
        property_handler_get_by_name(chain_comm_property_name_get(PROPERTY_HANDLER_CALIBRATION.id));
    property->from_json(&calibration, json);
}