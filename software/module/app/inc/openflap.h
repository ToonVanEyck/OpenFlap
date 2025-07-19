#pragma once

#include "chain_comm.h"
#include "config.h"
#include "flash.h"
#include "peripherals.h"
#include "platform.h"

#include <stdint.h>

#ifndef GIT_VERSION
#define GIT_VERSION "undefined"
#endif

/** Struct with helper variables. */
typedef struct openflap_ctx_tag {
    uint8_t flap_setpoint;                /**< The desired position of flap wheel. */
    uint8_t flap_position;                /**< The current position of flap wheel. */
    uint8_t flap_distance;                /**< The distance between the current and target flap. */
    openflap_config_t config;             /**< The configuration data. */
    chain_comm_ctx_t chain_ctx;           /**< The chain communication context. */
    bool store_config;                    /**< Flag to store the configuration. */
    bool reboot;                          /**< Flag to indicate the module must perform a system reboot. */
    bool motor_active;                    /**< Flag to indicate if the motor is busy. */
    bool extend_revolution;               /**< Flag to indicate if the motor must make at least on revolution. */
    uint32_t motor_active_timeout_tick;   /**< The time when the motor busy timeout will occur. */
    bool comms_active;                    /**< Flag to indicate if the communication is busy. */
    uint32_t comms_active_timeout_tick;   /**< The time when the communication busy timeout will occur. */
    uint32_t motor_backspin_timeout_tick; /**< When the distance reaches 0, reverse motor until timeout. */
    uint16_t ir_tick_cnt;                 /**< Counter for determining IR sensor state. */
} openflap_ctx_t;

/**
 * \brief Calculate a PWM duty cycle based on the distance between the setpoint and the encoder position.
 *
 * \param[in] cfg The motion configuration parameters.
 * \param[in] distance The distance between the setpoint and the encoder position.
 * \return The calculated PWM duty cycle.
 */
uint8_t pwm_duty_cycle_calc(const openflap_motion_config_t *cfg, uint8_t distance);

/**
 * \brief Map the index to a range between 0 and #SYMBOL_CNT.
 *
 * \param index the index.
 * \return The wrapped index.
 */
inline uint8_t flapIndex_wrap_calc(int8_t index)
{
    return (((index % SYMBOL_CNT) + SYMBOL_CNT) % SYMBOL_CNT);
}

/**
 * \brief Calculate the encoder position based on the ADC data. The updated position is stored in the context.
 *
 * \param[inout] ctx A pointer to the openflap context.
 * \param[in] encoder_states The current states of the encoder inputs.
 */
void encoder_position_update(openflap_ctx_t *ctx, const encoder_states_t *encoder_reading);

/**
 * \brief Update the distance between the current and target flap.
 *
 * \param[inout] ctx A pointer to the openflap context.
 */
void distance_update(openflap_ctx_t *ctx);

/**
 * \brief Update the internal state variable that is monitoring the motor state.
 *
 * \param[inout] ctx A pointer to the openflap context.
 */
void motor_state_update(openflap_ctx_t *ctx);

/**
 * \brief Update the internal state variable that is monitoring the communication state.
 *
 * \param[inout] ctx A pointer to the openflap context.
 */
void comms_state_update(openflap_ctx_t *ctx);

/**
 * \brief Set the motor speed based on the distance between the current and target flap.
 *
 * \param[inout] ctx A pointer to the openflap context.
 */
void from_distance_motor_set(openflap_ctx_t *ctx);

/**
 * \brief Set the motor mode and speed.
 *
 * \param[in] mode The motor mode.
 * \param[in] speed The motor speed, unused when \p mode is #MOTOR_FORWARD or #MOTOR_REVERSE.
 */
void motor_set(motor_mode_t mode, uint8_t speed);

/**
 * \brief Run the motor forwards.
 *
 * \param[in] speed The speed of the motor.
 */
inline void motor_forward(uint8_t speed)
{
    motor_set(MOTOR_FORWARD, speed);
}

/**
 * \brief Run the motor in reverse.
 *
 * \param[in] speed The speed of the motor.
 */
inline void motor_reverse(uint8_t speed)
{
    motor_set(MOTOR_REVERSE, speed);
}

/**
 * \brief Let the motor idle / freewheel.
 */
inline void motor_idle(void)
{
    motor_set(MOTOR_IDLE, 0);
}

/**
 * \brief Actively brake the motor.
 */
inline void motor_brake(void)
{
    motor_set(MOTOR_BRAKE, 0);
}

/**
 * \brief Increment the encoder position.
 *
 * \param[inout] ctx A pointer to the openflap context.
 */
void encoder_increment(openflap_ctx_t *ctx);

/**
 * \brief Zero the encoder position.
 *
 * \param[inout] ctx A pointer to the openflap context.
 */
void encoder_zero(openflap_ctx_t *ctx);

/**
 * \brief Get the position determined by the encoder taking in to account the encoder offset.
 *
 * \param[in] ctx A pointer to the openflap context.
 *
 * \return The position.
 */
uint8_t flap_postion_get(openflap_ctx_t *ctx);