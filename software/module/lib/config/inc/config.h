#pragma once
#include <stdbool.h>
#include <stdint.h>

/** The number of flaps in the split flap module. */
#define SYMBOL_CNT (48)

/** The number of IR sensors. */
#define SENS_CNT (6)

/** The minimum and maximum PWM values. */
#define ABS_MIN_PWM (35)
#define ABS_MAX_PWM (200)

typedef struct {
    uint8_t min_pwm;           /**< The minimum PWM value. */
    uint8_t max_pwm;           /**< The maximum PWM value. */
    uint8_t min_ramp_distance; /**< The distance below which the motor will run at minimum PWM. */
    uint8_t max_ramp_distance; /**< The distance above which the motor will run at maximum PWM. */
} openflap_motion_config_t;

/** Configuration data for NVM storage. */
typedef struct openflap_config_tag {
    uint8_t encoder_offset; /**< Offset of the encoder compared to the actual symbol index. */
    struct {
        uint16_t lower;              /**< Value for the lower threshold. */
        uint16_t upper;              /**< Value for the upper threshold. */
    } ir_threshold;                  /**< Sensor thresholds for IR sensors. */
    uint8_t base_speed;              /**< Base speed of the flap wheel. */
    uint32_t symbol_set[SYMBOL_CNT]; /**< An array of all supported symbols. */
    bool ota_completed;              /**< Flag to indicate that the OTA process is completed. */
    uint8_t minimum_rotation;        /**< Add a complete rotation if the minimum distance between the current flap and
                                         destination flap is not met. */
    struct {
        uint32_t foreground;         /**< The foreground color of the flaps. */
        uint32_t background;         /**< The background color of the flaps. */
    } color;                         /**< The color of the flaps. */
    openflap_motion_config_t motion; /**< The motion parameters of the flaps. */
    uint8_t stepper_step_offset;     /**< The offset between the homing mark and a flap position in steps. */
} openflap_config_t;

/** Load the config form NVM. */
void configLoad(openflap_config_t *config);

/** Store the config in NVM. */
void configStore(openflap_config_t *config);

/** Print the config to RTT. */
void configPrint(openflap_config_t *config);