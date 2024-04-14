#include "flash.h"

static void flashErase(uint32_t address);

void flashRead(uint32_t address, flashPage_t *data, uint8_t page_cnt)
{
    memcpy(data, (void *)address, page_cnt * sizeof(flashPage_t));
}

void flashWrite(uint32_t address, flashPage_t *data, uint8_t page_cnt)
{
    HAL_FLASH_Unlock();
    if (!(address % FLASH_SECTOR_SIZE)) {
        flashErase(address);
    }
    uint32_t flash_program_start = address;
    uint32_t flash_program_end = (address + page_cnt * FLASH_PAGE_SIZE);
    flashPage_t *src = (flashPage_t *)data;

    while (flash_program_start < flash_program_end) {
        // Write to flash
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_PAGE, flash_program_start, (uint32_t *)src) == HAL_OK) {
            // Move flash point to next page
            flash_program_start += FLASH_PAGE_SIZE;
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