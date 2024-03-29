#include "flash.h"

/* Use last flash sector for NVM config. */
#define FLASH_NVM_START_ADDR ((FLASH_END + 1) - (1 * FLASH_SECTOR_SIZE))

static void flashErase(void);

void flashRead(flashPage_t *data, uint8_t page_cnt)
{
    memcpy(data, (void *)FLASH_NVM_START_ADDR, page_cnt * sizeof(flashPage_t));
}

void flashWrite(flashPage_t *data, uint8_t page_cnt)
{
    HAL_FLASH_Unlock();
    flashErase();
    uint32_t flash_program_start = FLASH_NVM_START_ADDR;
    uint32_t flash_program_end = (FLASH_NVM_START_ADDR + page_cnt * FLASH_PAGE_SIZE);
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

static void flashErase(void)
{
    uint32_t SECTORError = 0;
    FLASH_EraseInitTypeDef EraseInitStruct;
    // Erase type = sector
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORERASE;
    // Erase address start
    EraseInitStruct.SectorAddress = FLASH_NVM_START_ADDR;
    // Number of sectors
    EraseInitStruct.NbSectors = 1;
    // Erase
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError) != HAL_OK) {
        APP_ErrorHandler();
    }
}