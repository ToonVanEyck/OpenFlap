#pragma once

#include "py32f0xx_hal.h"
#include <stdint.h>

typedef struct flashPage_t {
    uint32_t b32[FLASH_PAGE_SIZE / 4];
} flashPage_t;

/** Read from flash memory. */
void flashRead(uint32_t address, uint8_t *data, uint32_t size);

/** Write to flash memory. */
void flashWrite(uint32_t address, uint8_t *data, uint32_t size);