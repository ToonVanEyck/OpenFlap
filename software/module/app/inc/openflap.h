#pragma once

#include "chain_comm.h"
#include "config.h"
#include "flash.h"
#include "platform.h"

extern TIM_HandleTypeDef motorPwmHandle;

/** Struct with helper variables. */
typedef struct openflap_ctx_tag {
    uint8_t flap_setpoint;              /**< The desired position of flap wheel. */
    uint8_t flap_position;              /**< The current position of flap wheel. */
    openflap_config_t config;           /**< The configuration data. */
    chain_comm_ctx_t chain_ctx;         /**< The chain communication context. */
    bool store_config;                  /**< Flag to store the configuration. */
    bool reboot;                        /**< Flag to indicate the module must preform a system reboot. */
    bool motor_active;                  /**< Flag to indicate if the motor is busy. */
    bool extend_revolution;           /**< Flag to indicate if the motor must make at least on revolution. */
    uint32_t motor_active_timeout_tick; /**< The time when the motor busy timeout will occur. */
    bool comms_active;                  /**< Flag to indicate if the communication is busy. */
    uint32_t comms_active_timeout_tick; /**< The time when the communication busy timeout will occur. */
    uint16_t ir_tick_cnt;               /**< Counter for determining IR sensor state. */
} openflap_ctx_t;

/** Motor operation modes. */
typedef enum motorMode_tag {
    MOTOR_IDLE,    /**< Let the motor idle / freewheel. */
    MOTOR_BRAKE,   /**< Actively brake the motor. */
    MOTOR_FORWARD, /**< Run the motor forwards. */
    MOTOR_REVERSE, /**< Run the motor in reverse. */
} motorMode_t;

/**
 * \brief Calculate a PWM duty cycle based on the distance between the setpoint and the encoder position.
 *
 * \param[in] distance The distance between the setpoint and the encoder position.
 * \return The calculated PWM duty cycle.
 */
uint8_t pwmDutyCycleCalc(uint8_t distance);

/**
 * \brief Map the index to a range between 0 and #SYMBOL_CNT.
 *
 * \param index the index.
 * \return The wrapped index.
 */
inline uint8_t flapIndexWrapCalc(int8_t index)
{
    return (((index % SYMBOL_CNT) + SYMBOL_CNT) % SYMBOL_CNT);
}

/**
 * \brief Calculate the encoder position based on the ADC data. The updated position is stored in the context.
 *
 * \param[inout] ctx A pointer to the openflap context.
 * \param[in] adc_data The ADC data of the IR sensors.
 */
void encoderPositionUpdate(openflap_ctx_t *ctx, uint32_t *adc_data);

/**
 * \brief Generate a random seed based on the ADC data.
 *
 * \param[in] adc_data The ADC data of the IR sensors.
 *
 * \return The generated random seed.
 */
uint8_t getAdcBasedRandSeed(uint32_t *adc_data);

/**
 * \brief Update the internal state variable that is monitoring the motor state.
 *
 * \param[inout] ctx A pointer to the openflap context.
 */
void updateMotorState(openflap_ctx_t *ctx);

/**
 * \brief Update the internal state variable that is monitoring the communication state.
 *
 * \param[inout] ctx A pointer to the openflap context.
 */
void updateCommsState(openflap_ctx_t *ctx);

/**
 * \brief Set the motor mode and speed.
 *
 * \param[in] mode The motor mode.
 * \param[in] speed The motor speed, unused when \p mode is #MOTOR_FORWARD or #MOTOR_REVERSE.
 */
void setMotor(motorMode_t mode, uint8_t speed);

/**
 * \brief Run the motor forwards.
 *
 * \param[in] speed The speed of the motor.
 */
inline void motorForward(uint8_t speed)
{
    setMotor(MOTOR_FORWARD, speed);
}

/**
 * \brief Run the motor in reverse.
 *
 * \param[in] speed The speed of the motor.
 */
inline void motorReverse(uint8_t speed)
{
    setMotor(MOTOR_REVERSE, speed);
}

/**
 * \brief Let the motor idle / freewheel.
 */
inline void motorIdle(void)
{
    setMotor(MOTOR_IDLE, 0);
}

/**
 * \brief Actively brake the motor.
 */
inline void motorBrake(void)
{
    setMotor(MOTOR_BRAKE, 0);
}