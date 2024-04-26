#include "config.h"
#include "memory_map.h"
#include "py32f0xx_hal.h"
#include <stdint.h>

#define CRC_VALID (0)
#define APP_INDEX_SIZE (2)

static openflap_config_t config;
static uint32_t app_size;
static uint32_t *app_start;

typedef struct vector_table_tag {
    uint32_t initial_stack_pointer;
    void (*reset_handler)(void);
} vector_table_t;

void jump_to_app(uint8_t app_index)
{
    vector_table_t *app_vector_table = (vector_table_t *)(app_start + (app_index * app_size));
    __disable_irq();
    __set_CONTROL(0);
    __set_MSP(app_vector_table->initial_stack_pointer);
    SCB->VTOR = app_vector_table->initial_stack_pointer;
    app_vector_table->reset_handler();
}

CRC_HandleTypeDef CrcHandle;

int main(void)
{
    HAL_Init();

    /* Set App size */
    app_size = ((uint32_t)&__FLASH_APP_SIZE__) / 4 + ((uint32_t)&__FLASH_CS_SIZE__) / 4;
    /* Set App memory */
    app_start = (uint32_t *)&__FLASH_APP_START__;

    /* Load config */
    configLoad(&config);

    /* Init CRC peripheral */
    CrcHandle.Instance = CRC;
    if (HAL_CRC_Init(&CrcHandle) != HAL_OK) {
    }

    /* Calculate checksums of apps. */
    uint32_t app_crc[APP_INDEX_SIZE] = {0xFFFFFFFF};
    for (uint8_t i = 0; i < APP_INDEX_SIZE; i++) {
        app_crc[i] = HAL_CRC_Calculate(&CrcHandle, app_start + (i * app_size), app_size);
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
