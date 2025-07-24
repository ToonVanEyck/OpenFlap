#include "flash.h"

#include "py32f0xx_ll_flash.h"

#include <string.h>

static void flashErase(uint32_t address);

void flashRead(uint32_t address, uint8_t *data, uint32_t size)
{
    memcpy(data, (void *)address, size);
}

void flashWrite(uint32_t address, uint8_t *data, uint32_t size)
{
    uint32_t flash_addr     = address;
    uint32_t flash_end      = (address + size);
    uint32_t size_remaining = flash_end - flash_addr;
    flashPage_t *src        = (flashPage_t *)data;
    flashPage_t data_page;

    // Unlock Flash for write access
    LL_FLASH_Unlock();

    while (size_remaining) {
        // Erase when starting a new flash sector
        if (!(flash_addr % FLASH_SECTOR_SIZE)) {
            flashErase(flash_addr);
        }
        // Create temporary page to pad the page with 0xFF bytes.
        uint8_t write_size = (size_remaining < FLASH_PAGE_SIZE) ? size_remaining : FLASH_PAGE_SIZE;
        memset(&data_page, UINT32_MAX, sizeof(flashPage_t));
        memcpy(&data_page, src, write_size);
        // Write to flash
        if (LL_FLASH_Program(FLASH_TYPEPROGRAM_PAGE, flash_addr, (uint32_t *)&data_page) == SUCCESS) {
            // Move flash point to next page
            flash_addr += write_size;
            size_remaining -= write_size;
            // Move data point
            src++;
        }
    }
    LL_FLASH_Lock();
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
    LL_FLASHEx_Erase(&EraseInitStruct, &SECTORError);
}