#include "config.h"
#include "debug_io.h"
#include "flash.h"

extern uint32_t __FLASH_NVS_START__;
#define NVS_START_ADDR ((uint32_t)&__FLASH_NVS_START__)

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
    debug_io_log_info("Config:\n");
    debug_io_log_info("Encoder offset: %d\n", config->encoder_offset);
    debug_io_log_info("IR limits:\n");
    for (int i = 0; i < SENS_CNT; i++) {
        debug_io_log_info("IR thresholds: %d %d\n", config->ir_threshold.lower, config->ir_threshold.upper);
    }
    debug_io_log_info("Base speed: %d\n", config->base_speed);
    debug_io_log_info("Symbol set:\n");
    for (int i = 0; i < SYMBOL_CNT; i++) {
        debug_io_log_info("%s ", &config->symbol_set[i]);
    }
    debug_io_log_info("Minimum rotation: %d\n", config->minimum_rotation);
    debug_io_log_info("Foreground Color: %d\n", config->color.foreground);
    debug_io_log_info("Background Color: %d\n", config->color.background);
    debug_io_log_info("Motion config: %d %d %d %d\n", config->motion.min_pwm, config->motion.max_pwm,
                      config->motion.min_ramp_distance, config->motion.max_ramp_distance);
    debug_io_log_info("\n");
}