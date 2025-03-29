#include "config.h"
#include "debug_io.h"
#include "flash.h"
#include "memory_map.h"

void configLoad(openflap_config_t *config)
{
    flashRead(NVS_START_ADDR, (uint8_t *)config, sizeof(openflap_config_t));
}

void configStore(openflap_config_t *config)
{
    flashWrite(NVS_START_ADDR, (uint8_t *)config, sizeof(openflap_config_t));
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
        debug_io_log_info("IR thresholds: %d %d\n", config->ir_threshold.lower, config->ir_threshold.upper);
        HAL_Delay(d);
    }
    debug_io_log_info("Base speed: %d\n", config->base_speed);
    HAL_Delay(d);
    debug_io_log_info("Symbol set:\n");
    HAL_Delay(d);
    for (int i = 0; i < SYMBOL_CNT; i++) {
        debug_io_log_info("%s ", &config->symbol_set[i]);
        HAL_Delay(d);
    }
    debug_io_log_info("Minimum rotation: %d\n", config->minimum_rotation);
    HAL_Delay(d);
    debug_io_log_info("Foreground Color: %d\n", config->color.foreground);
    HAL_Delay(d);
    debug_io_log_info("Background Color: %d\n", config->color.background);
    HAL_Delay(d);
    debug_io_log_info("Motion config: %d %d %d %d\n", config->motion.min_pwm, config->motion.max_pwm,
                      config->motion.min_ramp_distance, config->motion.max_ramp_distance);
    HAL_Delay(d);
    debug_io_log_info("\n");
}