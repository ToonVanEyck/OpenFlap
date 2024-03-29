#pragma once

#include "flash.h"

/** Sensor threshold values. */
typedef struct ir_sens_threshold_tag {
    uint16_t ir_low;
    uint16_t ir_high;
} ir_sens_threshold_t;

/** Configuration data for NVM storage. */
typedef struct openflap_config_tag {
    uint8_t encoder_offset;                  /**< Offset of the encoder compared to the actual symbol index. */
    ir_sens_threshold_t ir_limits[SENS_CNT]; /**< Sensor thresholds for each IR sensor. */
    uint8_t vtrim;                           /**< Virtual trim setting. */
    uint8_t base_speed;                      /**< Base speed of the flap wheel. */
    uint32_t symbol_set[SYMBOL_CNT];         /**< An array of all supported symbols. */
} openflap_config_t;

/** Load the config form NVM. */
void configLoad(openflap_config_t *config);

/** Store the config in NVM. */
void configStore(openflap_config_t *config);

/** Print the config to RTT. */
void configPrint(openflap_config_t *config);