#include "config.h"
#include "flash.h"

void configLoad(openflap_config_t *config)
{
    flashRead((flashPage_t *)config, 1 + (sizeof(openflap_config_t) / sizeof(flashPage_t)));
}

void configStore(openflap_config_t *config)
{
    flashWrite((flashPage_t *)config, 1 + (sizeof(openflap_config_t) / sizeof(flashPage_t)));
}

void configPrint(openflap_config_t *config)
{
    uint8_t d = 25;
    SEGGER_RTT_printf(0, "Config:\n");
    HAL_Delay(d);
    SEGGER_RTT_printf(0, "Encoder offset: %d\n", config->encoder_offset);
    HAL_Delay(d);
    SEGGER_RTT_printf(0, "IR limits:\n");
    HAL_Delay(d);
    for (int i = 0; i < SENS_CNT; i++) {
        SEGGER_RTT_printf(0, "IR sensor %d:  %d\n", i, config->ir_limits[i]);
        HAL_Delay(d);
    }
    SEGGER_RTT_printf(0, "Vtrim: %d\n", config->vtrim);
    HAL_Delay(d);
    SEGGER_RTT_printf(0, "Base speed: %d\n", config->base_speed);
    HAL_Delay(d);
    SEGGER_RTT_printf(0, "Symbol set:\n");
    HAL_Delay(d);
    for (int i = 0; i < SYMBOL_CNT; i++) {
        SEGGER_RTT_printf(0, "%s ", &config->symbol_set[i]);
        HAL_Delay(d);
    }
}