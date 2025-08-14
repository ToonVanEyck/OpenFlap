#include "py32f0xx.h"
#include "py32f0xx_bsp_clock.h"
#include "py32f0xx_ll_bus.h"
#include "py32f0xx_ll_crc.h"

#include "flash.h"
#include "memory_map.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define CRC_VALID (0)

typedef struct vector_table_tag {
    uint32_t initial_stack_pointer;
    void (*reset_handler)(void);
} vector_table_t;

/**
 * \brief Jump to the main app.
 */
void jump_to_app(void)
{
    vector_table_t *app_vector_table = (vector_table_t *)APP_N_START_PTR(MAIN_APP);
    // Set stack pointer.
    __set_MSP(app_vector_table->initial_stack_pointer);
    // Disable all interrupts.
    memset((uint32_t *)NVIC->ICER, 0xFF, sizeof(NVIC->ICER));
    memset((uint32_t *)NVIC->ICPR, 0xFF, sizeof(NVIC->ICPR));
    // Jump to reset handler.
    app_vector_table->reset_handler();
}

uint32_t calculate_crc(uint32_t *data, size_t size)
{
    LL_CRC_ResetCRCCalculationUnit(CRC);
    for (size_t i = 0; i < size; ++i) {
        LL_CRC_FeedData32(CRC, data[i]);
    }
    return LL_CRC_ReadData32(CRC);
}

/**
 * The bootloader checks if a new app has been written in the flash memory. If there is, and it's CRC is valid, it is
 * copied to the main app partition. The main app is then checked for validity and if it is valid, the bootloader jumps
 * to the main app.
 */
int main(void)
{
    BSP_RCC_HSI_8MConfig();                            // Initialize the system clock to 8 MHz
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_CRC); // Enable CRC clock

    bool main_app_valid = false;
    bool new_app_valid  = false;

    /* Check if there is a valid new app. */
    new_app_valid = (CRC_VALID == calculate_crc(APP_N_START_PTR(NEW_APP), APP_SIZE / 4));

    /* Copy new app to main app memory. */
    if (new_app_valid) {
        flash_write((uint32_t)APP_N_START_PTR(MAIN_APP), (uint8_t *)APP_N_START_PTR(NEW_APP), APP_SIZE);

        /* Read the last flash page of the app, this page contains the checksum in the 4 last bytes, invert them and
         * write them back. This prevents the bootloader from re-copying the app on next boot. */
        const uint32_t last_page_addr = (uint32_t)APP_N_START_PTR(NEW_APP) + APP_SIZE - FLASH_PAGE_SIZE;
        flash_page_t flash_page;
        flash_read(last_page_addr, (uint8_t *)&flash_page, FLASH_PAGE_SIZE);
        flash_page.b32[FLASH_PAGE_SIZE / 4 - 1] = ~flash_page.b32[FLASH_PAGE_SIZE / 4 - 1];
        flash_write(last_page_addr, (uint8_t *)&flash_page, FLASH_PAGE_SIZE);
    }

    /* Verify the main app. */
    main_app_valid = (CRC_VALID == calculate_crc(APP_N_START_PTR(MAIN_APP), APP_SIZE / 4));

    /* Jump to app. */
    if (main_app_valid) {
        jump_to_app();
    }

    /* Should never reach this loop. */
    while (1) {
    }
}
