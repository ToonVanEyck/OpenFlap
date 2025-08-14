#pragma once

#include "py32f0xx.h"
#include <stdint.h>

typedef struct flash_page_t {
    uint32_t b32[FLASH_PAGE_SIZE / 4];
} flash_page_t;

/** Read from flash memory. */
void flash_read(uint32_t address, uint8_t *data, uint32_t size);

/** Write to flash memory. */
void flash_write(uint32_t address, uint8_t *data, uint32_t size);