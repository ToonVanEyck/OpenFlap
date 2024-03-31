#include "config.h"

void configLoad(openflap_config_t *config)
{
    config->ir_limits[0] = (ir_sens_threshold_t){.ir_low = 450, .ir_high = 450};
    config->ir_limits[1] = (ir_sens_threshold_t){.ir_low = 450, .ir_high = 450};
    config->ir_limits[2] = (ir_sens_threshold_t){.ir_low = 450, .ir_high = 450};
    config->ir_limits[3] = (ir_sens_threshold_t){.ir_low = 450, .ir_high = 450};
    config->ir_limits[4] = (ir_sens_threshold_t){.ir_low = 450, .ir_high = 450};
    config->ir_limits[5] = (ir_sens_threshold_t){.ir_low = 450, .ir_high = 450};

    config->encoder_offset = 0;
    memcpy(config->symbol_set,
           (uint32_t[]){
               *(uint32_t *)(uint8_t[4]){" "}, *(uint32_t *)(uint8_t[4]){"A"}, *(uint32_t *)(uint8_t[4]){"B"},
               *(uint32_t *)(uint8_t[4]){"C"}, *(uint32_t *)(uint8_t[4]){"D"}, *(uint32_t *)(uint8_t[4]){"E"},
               *(uint32_t *)(uint8_t[4]){"F"}, *(uint32_t *)(uint8_t[4]){"G"}, *(uint32_t *)(uint8_t[4]){"H"},
               *(uint32_t *)(uint8_t[4]){"I"}, *(uint32_t *)(uint8_t[4]){"J"}, *(uint32_t *)(uint8_t[4]){"K"},
               *(uint32_t *)(uint8_t[4]){"L"}, *(uint32_t *)(uint8_t[4]){"M"}, *(uint32_t *)(uint8_t[4]){"N"},
               *(uint32_t *)(uint8_t[4]){"O"}, *(uint32_t *)(uint8_t[4]){"P"}, *(uint32_t *)(uint8_t[4]){"Q"},
               *(uint32_t *)(uint8_t[4]){"R"}, *(uint32_t *)(uint8_t[4]){"S"}, *(uint32_t *)(uint8_t[4]){"T"},
               *(uint32_t *)(uint8_t[4]){"U"}, *(uint32_t *)(uint8_t[4]){"V"}, *(uint32_t *)(uint8_t[4]){"W"},
               *(uint32_t *)(uint8_t[4]){"X"}, *(uint32_t *)(uint8_t[4]){"Y"}, *(uint32_t *)(uint8_t[4]){"Z"},
               *(uint32_t *)(uint8_t[4]){"0"}, *(uint32_t *)(uint8_t[4]){"1"}, *(uint32_t *)(uint8_t[4]){"2"},
               *(uint32_t *)(uint8_t[4]){"3"}, *(uint32_t *)(uint8_t[4]){"4"}, *(uint32_t *)(uint8_t[4]){"5"},
               *(uint32_t *)(uint8_t[4]){"6"}, *(uint32_t *)(uint8_t[4]){"7"}, *(uint32_t *)(uint8_t[4]){"8"},
               *(uint32_t *)(uint8_t[4]){"9"}, *(uint32_t *)(uint8_t[4]){"€"}, *(uint32_t *)(uint8_t[4]){"$"},
               *(uint32_t *)(uint8_t[4]){"!"}, *(uint32_t *)(uint8_t[4]){"?"}, *(uint32_t *)(uint8_t[4]){"."},
               *(uint32_t *)(uint8_t[4]){","}, *(uint32_t *)(uint8_t[4]){":"}, *(uint32_t *)(uint8_t[4]){"/"},
               *(uint32_t *)(uint8_t[4]){"@"}, *(uint32_t *)(uint8_t[4]){"#"}, *(uint32_t *)(uint8_t[4]){"&"},
           },
           sizeof(config->symbol_set));
    // flashRead((flashPage_t *)config, 1 + (sizeof(openflap_config_t) / sizeof(flashPage_t)));
}

void configStore(openflap_config_t *config)
{
    flashWrite((flashPage_t *)&config, 1 + (sizeof(openflap_config_t) / sizeof(flashPage_t)));
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
        SEGGER_RTT_printf(0, "IR sensor %d: low: %d, high: %d\n", i, config->ir_limits[i].ir_low,
                          config->ir_limits[i].ir_high);
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