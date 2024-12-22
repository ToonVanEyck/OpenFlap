#include "calibration_property.h"
#include "chain_comm_abi.h"
#include "esp_err.h"
#include "esp_system.h"
#include "memory_checks.h"
#include "properties.h"
#include "unity.h"

TEST_CASE("Test Get Property Handler by id", "[properties][qemu]")
{
    const property_handler_t *property = property_handler_get_by_id(PROPERTY_CALIBRATION);
    TEST_ASSERT_NOT_NULL(property);
    TEST_ASSERT_EQUAL_STRING("calibration", chain_comm_property_name_by_id(property->id));
}

TEST_CASE("Test Get Property Handler by id not found", "[properties][qemu]")
{
    const property_handler_t *property = property_handler_get_by_id(PROPERTIES_MAX);
    TEST_ASSERT_NULL(property);
}
