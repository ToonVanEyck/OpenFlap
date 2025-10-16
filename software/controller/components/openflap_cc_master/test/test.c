#include "chain_comm_esp.h"
#include "memory_checks.h"
#include "openflap_display.h"
#include "property_handler.h"
#include "unity.h"

#define TAG      "CHAIN_COMM_TEST"
#define SYMBOL_€ (0x00ac82e2) // € symbol

TEST_CASE("Test chain comm write all character_set", "[chain_comm][target]")
{
    of_display_t display;
    TEST_ASSERT_EQUAL(ESP_OK, of_display_init(&display));

    of_cc_master_ctx_t ctx;
    TEST_ASSERT_EQUAL(ESP_OK, of_cc_master_init(&ctx, &display));

    /* Create a display with one module. */
    of_display_resize(&display, 1);

    /* Set a character set */
    uint32_t character_set_data[] = {
        (uint32_t)' ', (uint32_t)'A', (uint32_t)'B', (uint32_t)'C', (uint32_t)'D', (uint32_t)'E', (uint32_t)'F',
        (uint32_t)'G', (uint32_t)'H', (uint32_t)'I', (uint32_t)'J', (uint32_t)'K', (uint32_t)'L', (uint32_t)'M',
        (uint32_t)'N', (uint32_t)'O', (uint32_t)'P', (uint32_t)'Q', (uint32_t)'R', (uint32_t)'S', (uint32_t)'T',
        (uint32_t)'U', (uint32_t)'V', (uint32_t)'W', (uint32_t)'X', (uint32_t)'Y', (uint32_t)'Z', (uint32_t)'0',
        (uint32_t)'1', (uint32_t)'2', (uint32_t)'3', (uint32_t)'4', (uint32_t)'5', (uint32_t)'6', (uint32_t)'7',
        (uint32_t)'8', (uint32_t)'9', SYMBOL_€,      (uint32_t)'$', (uint32_t)'!', (uint32_t)'?', (uint32_t)'.',
        (uint32_t)',', (uint32_t)':', (uint32_t)'/', (uint32_t)'@', (uint32_t)'#', (uint32_t)'&',
    };
    display.modules[0].character_set.size          = 48;
    display.modules[0].character_set.character_set = (uint8_t *)character_set_data;

    /* Indicate a write all operation is required. */
    display_property_indicate_desynchronized(&display, PROPERTY_CHARACTER_SET, PROPERTY_SYNC_METHOD_WRITE);

    /* Wait for the display to synchronize. */
    of_display_synchronize(&display, 1000);

    /* Cleanup. */
    chain_comm_destroy(&ctx);
    display_destroy(&display);
}