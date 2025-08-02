/**
 * \file peripherals.h
 *
 * Used Peripherals:
 * - TIMER 1:
 *   - CH1 Input Capture of COMP2 event (motor commutator pulses).
 *   - CH3 & CH4 PWM output for motor control.
 *   - Tick timer for the system.
 * - TIMER 3: Quadrature encoder input.
 * - COMPARATOR 2: Motor commutator pulses.
 */

#pragma once

#include "py32f0xx_bsp_clock.h"
#include "py32f0xx_ll_adc.h"
#include "py32f0xx_ll_bus.h"
#include "py32f0xx_ll_comp.h"
#include "py32f0xx_ll_dma.h"
#include "py32f0xx_ll_flash.h"
#include "py32f0xx_ll_gpio.h"
#include "py32f0xx_ll_system.h"
#include "py32f0xx_ll_tim.h"
#include "py32f0xx_ll_usart.h"
#include "py32f0xx_ll_utils.h"

#include "rbuff.h"
#include "uart_driver.h"

#include <stdbool.h>
#include <stdint.h>

/** Motor operation modes. */
typedef enum {
    MOTOR_IDLE,    /**< Let the motor idle / freewheel. */
    MOTOR_BRAKE,   /**< Actively brake the motor. */
    MOTOR_FORWARD, /**< Run the motor forwards. */
    MOTOR_REVERSE, /**< Run the motor in reverse. */
} motor_mode_t;

/** Motor decay mode. */
typedef enum {
    MOTOR_DECAY_FAST, /**< Fast decay mode. */
    MOTOR_DECAY_SLOW  /**< Slow decay mode. */
} motor_decay_mode_t;

/** Type for controlling the motor. */
typedef struct {
    uint16_t speed;                /**< Motor speed (0-1000). */
    motor_mode_t mode;             /**< Motor operation mode. */
    motor_decay_mode_t decay_mode; /**< Motor decay mode. */
} motor_ctx_t;

typedef struct {
    uint32_t enc_raw_a;
    uint32_t enc_raw_b;
    uint32_t enc_raw_z;
    bool enc_a;
    bool enc_b;
    bool enc_z;
} encoder_states_t;

typedef struct {
    motor_ctx_t motor;             /**< Motor control context. */
    uart_driver_ctx_t uart_driver; /**< UART driver context. */
} peripherals_ctx_t;

/**
 * @brief Initializes all peripherals.
 */
void peripherals_init(peripherals_ctx_t *peripherals_ctx);

/**
 * @brief Deinitializes all peripherals.
 *
 * @note This function is not implemented.
 */
void peripherals_deinit(void);

/**
 * @brief Get the elapsed time in milliseconds since the last reset.
 *
 * @return The elapsed time in milliseconds.
 */
uint32_t get_tick_count(void);

/**
 * @brief Get the elapsed ticks since the last reset from the PWM timer.
 *
 * @return The elapsed ticks.
 */
uint32_t get_pwm_tick_count(void);

/**
 * @brief Sets a debug pin to a specific value.
 *
 * @param pin The pin number (0 to 3).
 * @param value The value to set (0 or 1).
 */
void debug_pin_set(uint8_t pin, bool value);

/**
 * @brief Toggles a debug pin.
 *
 * @param pin The pin number (0 to 3).
 */
void debug_pin_toggle(uint8_t pin);

/**
 * @brief Gets the current state of a debug pin.
 *
 * @param pin The pin number (0 to 3).
 * @return The state of the pin (true for high, false for low).
 */
bool debug_pin_get(uint8_t pin);

/**
 * @brief Updates the motor control based on the provided motor context.
 *
 * @param motor Pointer to the motor context containing speed and direction.
 */
void motor_control(motor_ctx_t *motor);

/**
 * @brief Check if the module is the last module in a column.
 *
 * @return true if it is the last module, false otherwise.
 */
bool is_column_end(void);

/**
 * @brief Get the current encoder states.
 *
 * @param encoder_states The encoder states structure to fill.
 * @param adc_lower_threshold The lower threshold for the ADC values.
 * @param adc_upper_threshold The upper threshold for the ADC values.
 *
 * @return The current encoder states.
 */
void encoder_states_get(encoder_states_t *states, uint16_t adc_lower_threshold, uint16_t adc_upper_threshold);

/**
 * @brief Enables or disables a secondary uart TX pin.
 *
 * @param enable_secondary_tx true to enable the secondary TX pin, false to disable it.
 */
void uart_tx_pin_update(bool enable_secondary_tx);