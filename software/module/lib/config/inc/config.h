#pragma once
#include <stdbool.h>
#include <stdint.h>

/** The number of flaps in the split flap module. */
#define SYMBOL_CNT (48)

/** The number of IR sensors. */
#define SENS_CNT (6)

/** Configuration data for NVM storage. */
typedef struct openflap_config_tag {
    uint8_t encoder_offset;          /**< Offset of the encoder compared to the actual symbol index. */
    uint16_t ir_limits[SENS_CNT];    /**< Sensor thresholds for each IR sensor. */
    uint8_t vtrim;                   /**< Virtual trim setting. */
    uint8_t base_speed;              /**< Base speed of the flap wheel. */
    uint32_t symbol_set[SYMBOL_CNT]; /**< An array of all supported symbols. */
    bool ota_completed;              /**< Flag to indicate that the OTA process is completed. */
    uint8_t random_seed;             /**< Random seed for the random number generator. */
    uint8_t minimum_distance;        /**< Add a complete rotation if the minimum distance between the current flap and
                                         destination flap is not met. */
} openflap_config_t;

/** Load the config form NVM. */
void configLoad(openflap_config_t *config);

/** Store the config in NVM. */
void configStore(openflap_config_t *config);

/** Print the config to RTT. */
void configPrint(openflap_config_t *config);