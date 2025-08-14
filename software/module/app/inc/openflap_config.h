/**
 * @file openflap_config.h
 *
 * This file contains the configuration data structure for the OpenFlap module.
 */

#pragma once

#include "hardware_setup.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t min_pwm;           /**< The minimum PWM value. */
    uint8_t max_pwm;           /**< The maximum PWM value. */
    uint8_t min_ramp_distance; /**< The distance below which the motor will run at minimum PWM. */
    uint8_t max_ramp_distance; /**< The distance above which the motor will run at maximum PWM. */
} of_motion_config_t;

/** Configuration data for NVM storage. */
typedef struct {
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
        uint32_t foreground;   /**< The foreground color of the flaps. */
        uint32_t background;   /**< The background color of the flaps. */
    } color;                   /**< The color of the flaps. */
    of_motion_config_t motion; /**< The motion parameters of the flaps. */
} of_config_t;