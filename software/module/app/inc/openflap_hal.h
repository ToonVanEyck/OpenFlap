/**
 * \file openflap_hal.h
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

#include "hardware_setup.h"
#include "openflap_config.h"
#include "rbuff.h"
#include "uart_driver.h"

#include <stdbool.h>
#include <stdint.h>

/** Type for controlling the motor. */
typedef struct {
    int16_t speed; /**< Motor speed and direction (-1000 to 1000). */
    int16_t decay; /**< Motor decay SLOW/MIXED/FAST (0 to 100). */
} of_hal_motor_ctx_t;

typedef struct {
    of_hal_motor_ctx_t motor;      /**< Motor control context. */
    uart_driver_ctx_t uart_driver; /**< UART driver context. */
} of_hal_ctx_t;

/**
 * @brief Initializes all peripherals.
 */
void of_hal_init(of_hal_ctx_t *of_hal_ctx);

/**
 * @brief Deinitializes all peripherals.
 *
 * @note This function is not implemented.
 */
void of_hal_deinit(void);

/**
 * @brief Get the elapsed time in milliseconds since the last reset.
 *
 * @return The elapsed time in milliseconds.
 */
uint32_t of_hal_tick_count_get(void);

/**
 * @brief Get the elapsed ticks since the last reset from the PWM timer.
 *
 * @return The elapsed ticks.
 */
uint32_t of_hal_pwm_tick_count_get(void);

/**
 * @brief Get the elapsed ticks since the last reset from the Sens timer.
 *
 * @return The elapsed ticks.
 */
uint32_t of_hal_sens_tick_count_get(void);

/**
 * @brief Sets a debug pin to a specific value.
 *
 * @param[in] pin The pin number (0).
 * @param[in] value The value to set (true/false).
 */
void of_hal_debug_pin_set(uint8_t pin, bool value);

/**
 * @brief Toggles a debug pin.
 *
 * @param[in] pin The pin number (0).
 */
void of_hal_debug_pin_toggle(uint8_t pin);

/**
 * @brief Gets the current state of a debug pin.
 *
 * @param[in] pin The pin number (0).
 * @return The state of the pin (true for high, false for low).
 */
bool of_hal_debug_pin_get(uint8_t pin);

/**
 * @brief Sets the state of the on-board LED.
 */
void of_hal_led_set(bool value);

/**
 * @brief Toggles the state of the on-board LED.
 */
void of_hal_led_toggle();

/**
 * @brief Updates the motor control based on the provided motor context.
 *
 * @param[in] speed A value from -1000 to +1000 where -1000 is the maximum reverse speed and +1000 is the maximum
 *                  forward speed.
 * @param[in] decay A value from 0 to 1000 representing the decay mode. Where 0 is full slow decay, 1000 is full fast
 *                  decay and the values between represent mixed decay.
 */
void of_hal_motor_control(int16_t speed, int16_t decay);

/**
 * @brief Check if the motor is currently running.
 *
 * @return true if the motor is running, false otherwise.
 */
bool of_hal_motor_is_running(void);

/**
 * @brief Check if the module is the last module in a column.
 *
 * @return true if it is the last module, false otherwise.
 */
bool of_hal_is_column_end(void);

/**
 * @brief Check if the module is receiving 12V power.
 *
 * @return true if it is receiving 12V power, false otherwise.
 */
bool of_hal_is_12V_ok(void);

/**
 * @brief Get the current encoder adc values.
 *
 * @param[out] encoder_values The encoder values array of size #ENCODER_CHANNEL_COUNT.
 *
 * @return The current encoder states.
 */
void of_hal_encoder_values_get(uint16_t *encoder_values);

/**
 * @brief Enables or disables a secondary uart TX pin.
 *
 * @param[in] enable_secondary_tx true to enable the secondary TX pin, false to disable it.
 */
void of_hal_uart_tx_pin_update(bool enable_secondary_tx);

/**
 * @brief Store the current configuration in the flash memory.
 *
 * @param[in] config Pointer to the configuration structure to store.
 */
void of_hal_config_store(of_config_t *config);

/**
 * @brief Load the configuration from the flash memory.
 *
 * @param[out] config Pointer to the configuration structure to load.
 */
void of_hal_config_load(of_config_t *config);

/**
 * @brief Decrease the timer frequency of the IR led to reduce power consumption.
 *
 * @param[in] idle true to set the timer to idle mode, false to set it to normal mode.
 */
void of_hal_ir_timer_idle_set(bool idle);