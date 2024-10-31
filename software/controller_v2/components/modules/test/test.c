#include "cJSON.h"
#include "esp_err.h"
#include "esp_system.h"
#include "memory_checks.h"
#include "modules.h"
#include "properties.h"
#include "unity.h"

#define TEST_JSON_VALID_PROPERTY                                                                                       \
    "{"                                                                                                                \
    "   \"module\": 1,"                                                                                                \
    "   \"calibration\": \"dummy\""                                                                                    \
    "}"

#define TEST_JSON_INVALID_PROPERTY                                                                                     \
    "{"                                                                                                                \
    "   \"module\": 1,"                                                                                                \
    "   \"unsupported_property\": \"value\""                                                                           \
    "}"

/* TODO mock the property handlers? */

TEST_CASE("Test set module properties from json, valid", "[modules][qemu]")
{
    module_t module;
    module.module_info = MODULE_INFO(MODULE_TYPE_SPLITFLAP, 0);
    cJSON *test_json   = cJSON_Parse(TEST_JSON_VALID_PROPERTY);
    TEST_ASSERT_NOT_NULL(test_json);
    uint8_t ret = module_properties_set_from_json(&module, test_json);
    TEST_ASSERT_EQUAL(1, ret);
    cJSON_Delete(test_json);
}

TEST_CASE("Test set module properties from json, not suported by module type", "[modules][qemu]")
{
    module_t module;
    module.module_info = MODULE_INFO(MODULE_TYPE_UNDEFINED, 0);
    cJSON *test_json   = cJSON_Parse(TEST_JSON_VALID_PROPERTY);
    TEST_ASSERT_NOT_NULL(test_json);
    uint8_t ret = module_properties_set_from_json(&module, test_json);
    TEST_ASSERT_EQUAL(0, ret);
    cJSON_Delete(test_json);
}

TEST_CASE("Test set module properties from json, not suported by controller", "[modules][qemu]")
{
    module_t module;
    module.module_info = MODULE_INFO(MODULE_TYPE_UNDEFINED, 0);
    cJSON *test_json   = cJSON_Parse(TEST_JSON_INVALID_PROPERTY);
    TEST_ASSERT_NOT_NULL(test_json);
    uint8_t ret = module_properties_set_from_json(&module, test_json);
    TEST_ASSERT_EQUAL(0, ret);
    cJSON_Delete(test_json);
}