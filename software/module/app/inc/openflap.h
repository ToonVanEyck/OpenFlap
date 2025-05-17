#pragma once

#include "chain_comm.h"
#include "config.h"
#include "flash.h"
#include "platform.h"
#include "stepper_driver.h"

#ifndef GIT_VERSION
#define GIT_VERSION "undefined"
#endif

/** Struct with helper variables. */
typedef struct openflap_ctx_tag {
    openflap_config_t config;             /**< The configuration data. */
    chain_comm_ctx_t chain_ctx;           /**< The chain communication context. */
    stepper_driver_ctx_t stepper_ctx;     /**< The stepper driver context. */
    bool store_config;                    /**< Flag to store the configuration. */
    bool reboot;                          /**< Flag to indicate the module must perform a system reboot. */
    bool extend_revolution;               /**< Flag to indicate if the motor must make at least on revolution. */
    bool comms_active;                    /**< Flag to indicate if the communication is busy. */
    uint32_t comms_active_timeout_tick;   /**< The time when the communication busy timeout will occur. */
    uint32_t motor_backspin_timeout_tick; /**< When the distance reaches 0, reverse motor until timeout. */
    uint16_t ir_tick_cnt;                 /**< Counter for determining IR sensor state. */
    bool home_found;                      /**< Flag to indicate if the home position has been found. */
} openflap_ctx_t;

/** Motor operation modes. */
typedef enum motorMode_tag {
    MOTOR_IDLE,               /**< Let the motor idle / freewheel. */
    MOTOR_BRAKE,              /**< Actively brake the motor. */
    MOTOR_FORWARD,            /**< Run the motor forwards. */
    MOTOR_REVERSE,            /**< Run the motor in reverse. */
    MOTOR_FORWARD_WITH_BREAK, /**< Run the motor forwards. But add a percentage of braking to allow slowing down. */
} motorMode_t;

/**
 * \brief Calculate a PWM duty cycle based on the distance between the setpoint and the encoder position.
 *
 * \param[in] cfg The motion configuration parameters.
 * \param[in] distance The distance between the setpoint and the encoder position.
 * \return The calculated PWM duty cycle.
 */
uint8_t pwmDutyCycleCalc(const openflap_motion_config_t *cfg, uint8_t distance);

/**
 * \brief Wrap a number around a base.
 *
 * \param number to wrap.
 * \param base the base to wrap around.
 *
 * \example Wrapping the number 11 around base 10 will return 1.
 *
 * \return The wrapped index.
 */
int32_t wrapNumberAroundBase(int32_t number, int32_t base);

/**
 * \brief Calculate the encoder position based on the ADC data. The updated position is stored in the context.
 *
 * \param[inout] ctx A pointer to the openflap context.
 * \param[in] adc_data The ADC data of the IR sensors.
 */
void homingPositionDecode(openflap_ctx_t *ctx, uint32_t *adc_data);

/**
 * \brief Calibrate the encoder thresholds based on the ADC data.
 *
 * \param[inout] ctx A pointer to the openflap context.
 * \param[in] adc_data The ADC data of the IR sensors.
 */
void encoderCalibration(openflap_ctx_t *ctx, uint32_t *adc_data);

/**
 * \brief Update the internal state variable that is monitoring the communication state.
 *
 * \param[inout] ctx A pointer to the openflap context.
 */
void updateCommsState(openflap_ctx_t *ctx);