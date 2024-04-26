#include "config.h"
#include "debug_io.h"
#include "flash.h"

/* Use last flash sector for NVM config. */
#define FLASH_NVM_START_ADDR ((FLASH_END + 1) - (1 * FLASH_SECTOR_SIZE))

void configLoad(openflap_config_t *config)
{
    flashRead(FLASH_NVM_START_ADDR, (flashPage_t *)config, 1 + (sizeof(openflap_config_t) / sizeof(flashPage_t)));
}

void configStore(openflap_config_t *config)
{
    flashWrite(FLASH_NVM_START_ADDR, (flashPage_t *)config, 1 + (sizeof(openflap_config_t) / sizeof(flashPage_t)));
}

void configPrint(openflap_config_t *config)
{
    uint8_t d = 25;
    debug_io_log_info("Config:\n");
    HAL_Delay(d);
    debug_io_log_info("Encoder offset: %d\n", config->encoder_offset);
    HAL_Delay(d);
    debug_io_log_info("IR limits:\n");
    HAL_Delay(d);
    for (int i = 0; i < SENS_CNT; i++) {
        debug_io_log_info("IR sensor %d:  %d\n", i, config->ir_limits[i]);
        HAL_Delay(d);
    }
    debug_io_log_info("Vtrim: %d\n", config->vtrim);
    HAL_Delay(d);
    debug_io_log_info("Base speed: %d\n", config->base_speed);
    HAL_Delay(d);
    debug_io_log_info("Symbol set:\n");
    HAL_Delay(d);
    for (int i = 0; i < SYMBOL_CNT; i++) {
        debug_io_log_info("%s ", &config->symbol_set[i]);
        HAL_Delay(d);
    }
    debug_io_log_info("\n");
}