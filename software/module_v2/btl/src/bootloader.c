#include "config.h"
#include "memory_map.h"
#include "py32f0xx_hal.h"
#include <stdint.h>
#include <string.h>

#define CRC_VALID (0)
#define APP_INDEX_SIZE (2)

#define APP_SIZE (((uint32_t) & __FLASH_APP_SIZE__) / 4 + ((uint32_t) & __FLASH_CS_SIZE__) / 4)
#define APP_START ((uint32_t *)&__FLASH_APP_START__)

static openflap_config_t config;

typedef struct vector_table_tag {
    uint32_t initial_stack_pointer;
    void (*reset_handler)(void);
} vector_table_t;

void jump_to_app(uint8_t app_index)
{
    vector_table_t *app_vector_table = (vector_table_t *)(APP_START + (app_index * APP_SIZE));
    /* Set stack pointer */
    __set_MSP(app_vector_table->initial_stack_pointer);
    /* Don't set vector table offset, vectors are copied to RAM. */
    // SCB->VTOR = (uint32_t)app_vector_table;
    memset((uint32_t *)NVIC->ICER, 0xFF, sizeof(NVIC->ICER));
    memset((uint32_t *)NVIC->ICPR, 0xFF, sizeof(NVIC->ICPR));
    /* Jump to reset handler */
    app_vector_table->reset_handler();
}

CRC_HandleTypeDef CrcHandle;

int main(void)
{
    HAL_Init();

    /* Load config */
    configLoad(&config);

    /* Init CRC peripheral */
    CrcHandle.Instance = CRC;
    if (HAL_CRC_Init(&CrcHandle) != HAL_OK) {
    }

    /* Calculate checksums of apps. */
    uint32_t app_crc[APP_INDEX_SIZE] = {0xFFFFFFFF};
    for (uint8_t i = 0; i < APP_INDEX_SIZE; i++) {
        app_crc[i] = HAL_CRC_Calculate(&CrcHandle, APP_START + (i * APP_SIZE), APP_SIZE);
    }

    uint8_t valid_app = APP_INDEX_SIZE;
    if (app_crc[config.active_app_index] == CRC_VALID) {
        /* Active app is valid */
        valid_app = config.active_app_index;
    } else {
        /* Active app is not valid. Rollback to a valid app. */
        for (uint8_t i = 0; i < APP_INDEX_SIZE; i++) {
            if (app_crc[i] == CRC_VALID) {
                valid_app = i;
            }
        }
    }

    /* Jump to app */
    if (valid_app < APP_INDEX_SIZE) {
        jump_to_app(valid_app);
    }

    /* Should never reach this loop. */
    while (1) {
    }
}
