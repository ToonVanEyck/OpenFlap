#include "flash.h"
#include <string.h>

static void flashErase(uint32_t address);

void flashRead(uint32_t address, uint8_t *data, uint32_t size)
{
    memcpy(data, (void *)address, size);
}

void flashWrite(uint32_t address, uint8_t *data, uint32_t size)
{
    HAL_FLASH_Unlock();
    if (!(address % FLASH_SECTOR_SIZE)) {
        flashErase(address);
    }
    uint32_t flash_addr = address;
    uint32_t flash_end = (address + size);
    flashPage_t *src = (flashPage_t *)data;

    while (flash_addr < flash_end) {
        // Create temporary page to pad the page with 0xFF bytes.
        uint32_t size_remaining = flash_end - flash_addr;
        flashPage_t data_page;
        memset(&data_page, UINT32_MAX, sizeof(flashPage_t));
        memcpy(&data_page, src, size_remaining < FLASH_PAGE_SIZE ? size_remaining : FLASH_PAGE_SIZE);
        // Write to flash
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_PAGE, flash_addr, (uint32_t *)&data_page) == HAL_OK) {
            // Move flash point to next page
            flash_addr += FLASH_PAGE_SIZE;
            // Move data point
            src++;
        }
    }
    HAL_FLASH_Lock();
}

static void flashErase(uint32_t address)
{
    uint32_t SECTORError = 0;
    FLASH_EraseInitTypeDef EraseInitStruct;
    // Erase type = sector
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORERASE;
    // Erase address start
    EraseInitStruct.SectorAddress = address;
    // Number of sectors
    EraseInitStruct.NbSectors = 1;
    // Erase
    HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError);
}