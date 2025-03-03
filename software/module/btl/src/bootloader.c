#include "config.h"
#include "flash.h"
#include "memory_map.h"
#include "py32f0xx_hal.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define CRC_VALID (0)

static openflap_config_t config;

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

CRC_HandleTypeDef CrcHandle;

/**
 * The bootloader checks if a new app has been written in the flash memory. If there is, and it's CRC is valid, it is
 * copied to the main app partition. The main app is then checked for validity and if it is valid, the bootloader jumps
 * to the main app.
 */
int main(void)
{
    HAL_Init();

    bool main_app_valid = false;
    bool new_app_valid  = false;

    // Load config.
    configLoad(&config);

    // Init CRC peripheral.
    CrcHandle.Instance = CRC;
    if (HAL_CRC_Init(&CrcHandle) != HAL_OK) {
    }

    if (config.ota_completed) {
        new_app_valid = (CRC_VALID == HAL_CRC_Calculate(&CrcHandle, APP_N_START_PTR(NEW_APP), APP_SIZE / 4));
        // Copy new app to main app.
        if (new_app_valid) {
            flashWrite((uint32_t)APP_N_START_PTR(MAIN_APP), (uint8_t *)APP_N_START_PTR(NEW_APP), APP_SIZE);
        }
        config.ota_completed = false;
        configStore(&config);
    }

    main_app_valid = (CRC_VALID == HAL_CRC_Calculate(&CrcHandle, APP_N_START_PTR(MAIN_APP), APP_SIZE / 4));
    // Jump to app.
    if (main_app_valid) {
        jump_to_app();
    }

    // Should never reach this loop.
    while (1) {
    }
}
