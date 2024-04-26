#pragma once

#include "py32f0xx_hal.h"
#include <stdint.h>

typedef struct flashPage_t {
    uint32_t b32[FLASH_PAGE_SIZE / 4];
} flashPage_t;

/** Read from flash memory. */
void flashRead(uint32_t address, flashPage_t *data, uint8_t page_cnt);

/** Write to flash memory. */
void flashWrite(uint32_t address, flashPage_t *data, uint8_t page_cnt);