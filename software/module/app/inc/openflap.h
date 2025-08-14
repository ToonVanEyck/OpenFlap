#pragma once

#include "chain_comm.h"
#include "hardware_setup.h"
#include "interpolation.h"
#include "openflap_hal.h"
#include "pid.h"

#include <stdint.h>

#ifndef GIT_VERSION
#define GIT_VERSION "undefined"
#endif

/** Struct with helper variables. */
typedef struct of_ctx_tag {
    of_config_t of_config;                /**< The configuration data. */
    of_hal_ctx_t of_hal;                  /**< The hardware abstraction layer context. */
    chain_comm_ctx_t chain_ctx;           /**< The chain communication context. */
    pid_ctx_t pid_ctx;                    /**< The PID controller context. */
    uint16_t flap_setpoint;               /**< The desired position of flap wheel. */
    uint16_t flap_position;               /**< The current position of flap wheel. */
    uint16_t flap_distance;               /**< The distance between the current and target flap. */
    bool store_config;                    /**< Flag to store the configuration. */
    bool reboot;                          /**< Flag to indicate the module must perform a system reboot. */
    bool motor_active;                    /**< Flag to indicate if the motor is busy. */
    bool comms_active;                    /**< Flag to indicate if the communication is busy. */
    bool extend_revolution;               /**< Flag to indicate if the motor must make at least on revolution. */
    uint32_t motor_active_timeout_tick;   /**< The time when the motor busy timeout will occur. */
    uint32_t comms_active_timeout_tick;   /**< The time when the communication busy timeout will occur. */
    uint32_t motor_backspin_timeout_tick; /**< When the distance reaches 0, reverse motor until timeout. */
    struct {
        uint16_t analog[ENCODER_CHANNEL_COUNT]; /**< ADC values for each encoder channel. */
        bool digital[ENCODER_CHANNEL_COUNT];    /**< Digital values for each encoder channel. */
    } encoder;
    struct {
        bool print_adc_values;           /**< Indicates if the ADC values should be printed. */
        bool motor_control_override;     /**< Indicates if the motor pwm should be calculated or fixed */
        bool rps_x100_setpoint_override; /**< Indicates if the motor speed setpoint should be calculated or fixed. */
    } debug_flags;                       /**< Debug flags. */
    // interp_ctx_t speed_pwm_interp_ctx;      /**< Context for speed PWM interpolation. */
    // interp_ctx_t distance_speed_interp_ctx; /**< Context for distance speed interpolation. */
    // interp_ctx_t speed_decay_pwm_interp_ctx; /**< Context for speed decay PWM interpolation. */
    // interp_ctx_t speed_decay_interp_ctx;     /**< Context for speed decay interpolation. */
    interp_ctx_t sdp_interpolation_ctx; /**< speed/decay to pwm interpolation context. */
    interp_ctx_t sd_interpolation_ctx;  /**< speed to decay interpolation context. */
    interp_ctx_t ds_interpolation_ctx;  /**< distance to speed interpolation context. */

    int32_t encoder_rps_x100_setpoint;       /**< The desired speed of the encoder in RPS x100. */
    int32_t encoder_rps_x100_actual;         /**< The actual speed of the encoder in RPS x100. */
    uint16_t flap_position_prev;             /**< The previous position of flap wheel. */
    uint32_t flap_position_change_tick_prev; /**< The last tick when the encoder value changed. */
} of_ctx_t;

/**
 * \brief Map the index to a range between 0 and #ENCODER_PULSES_PER_REVOLUTION.
 *
 * \param index the index.
 * \return The wrapped index.
 */
inline uint16_t flapIndex_wrap_calc(int16_t index)
{
    return (((index % ENCODER_PULSES_PER_REVOLUTION) + ENCODER_PULSES_PER_REVOLUTION) % ENCODER_PULSES_PER_REVOLUTION);
}

/**
 * @brief Convert the adc values of the encoder channels to boolean values based on the thresholds.
 *
 * @param[inout] ctx A pointer to the openflap context.
 */
void of_encoder_values_update(of_ctx_t *ctx);

/**
 * \brief Updates the encoder position based on the encoder data. The updated position is stored in the context.
 *
 * \note #of_encoder_values_update must be called prior to calling this function to ensure the encoder data is up to
 * date.
 *
 * \param[inout] ctx A pointer to the openflap context.
 */
void of_encoder_position_update(of_ctx_t *ctx);

/**
 * \brief Update the distance between the current and target flap.
 *
 * \param[inout] ctx A pointer to the openflap context.
 */
void distance_update(of_ctx_t *ctx);

/**
 * \brief Update the internal state variable that is monitoring the motor state.
 *
 * \param[inout] ctx A pointer to the openflap context.
 */
void motor_state_update(of_ctx_t *ctx);

/**
 * \brief Update the internal state variable that is monitoring the communication state.
 *
 * \param[inout] ctx A pointer to the openflap context.
 */
void comms_state_update(of_ctx_t *ctx);

/**
 * \brief Increment the encoder position.
 *
 * \param[inout] ctx A pointer to the openflap context.
 */
void encoder_increment(of_ctx_t *ctx);

/**
 * \brief Zero the encoder position.
 *
 * \param[inout] ctx A pointer to the openflap context.
 */
void encoder_zero(of_ctx_t *ctx);

/**
 * \brief Control loop for the motor.
 *
 * This function is called periodically to control the motor based on the current flap position and setpoint.
 *
 * \param[inout] ctx A pointer to the openflap context.
 * \param[in] cl_tick The current control loop tick.
 */
void motor_control_loop(of_ctx_t *ctx, uint32_t cl_tick);

/**
 * \brief Calculate the speed of the encoder based on the current and previous position.
 *
 * \param[inout] ctx A pointer to the openflap context.
 */
void of_encoder_speed_calc(of_ctx_t *ctx, uint32_t sens_tick);

/**
 * \brief Set the speed setpoint based on the current flap distance.
 *
 * \param[inout] ctx A pointer to the openflap context.
 */
void of_speed_setpoint_set_from_distance(of_ctx_t *ctx);
