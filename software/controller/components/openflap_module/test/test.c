#include "esp_err.h"
#include "esp_system.h"
#include "memory_checks.h"
#include "module.h"
#include "networking.h"
#include "unity.h"
#include "webserver.h"

#define TAG "MODULE_TEST"

/* TODO mock the property handlers? */

TEST_CASE("Test module_character_set_index_of_character", "[module][qemu]")
{

    module_t module = {
        .character_set = character_set_new(3),
    };
    memcmp(module.character_set->data, (uint8_t *)((char *) {"A\0\0\0B\0\0\0€\0"}), 12);

    uint8_t index;
    esp_err_t err;
    err = module_character_set_index_of_character(&module, &index, "A");
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(0, index);
    err = module_character_set_index_of_character(&module, &index, "€");
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(2, index);
    err = module_character_set_index_of_character(&module, &index, "!");
    TEST_ASSERT_EQUAL(ESP_FAIL, err);
}