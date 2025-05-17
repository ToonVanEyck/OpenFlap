#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "debug_io.h"
#include "py32f0xx_bsp_clock.h"
#include "py32f0xx_hal.h"

/**
 * \defgroup Column-end detection pin definitions.
 * @{
 */
#define COLEND_GPIO_PIN  GPIO_PIN_1
#define COLEND_GPIO_PORT GPIOA
/**
 * @}
 */

/**
 * \defgroup Absolute encoder IR definitions.
 * @{
 */
#define ENCODER_CHANNEL_CNT (1) /**< Number of tracks on the encoder. */

#define ENCODER_CHANNEL_Z (0) /**< Encoder Channel Z. */

/** List of ADC channels used by the encoder. */
#define ENCODER_ADC_CHANNEL_LIST                                                                                       \
    ((uint32_t[ENCODER_CHANNEL_CNT]) {                                                                                 \
        ADC_CHANNEL_3,                                                                                                 \
    })

#define ENCODER_ADC_GPIO_PINS (GPIO_PIN_3) /**< The pins used for ADC by the encoder. */
#define ENCODER_ADC_GPIO_PORT (GPIOA)      /**< ADC port. */
#define ENCODER_LED_GPIO_PIN  (GPIO_PIN_2) /**< Single pin driving the IR emitters of the IR sensors. */
#define ENCODER_LED_GPIO_PORT (GPIOA)      /**< IR emitter GPIO port. */
/**
 * @}
 */

/**
 * \defgroup Motor pin definitions.
 * @{
 */
#define STEPPER_GPIO_PORT    (GPIOA)    /**< Motor GPIO port. */
#define STEPPER_GPIO_A_P_PIN GPIO_PIN_4 /**< Stepper Motor Coil A Positive. */
#define STEPPER_GPIO_A_N_PIN GPIO_PIN_5 /**< Stepper Motor Coil A Negative. */
#define STEPPER_GPIO_B_P_PIN GPIO_PIN_6 /**< Stepper Motor Coil B Positive. */
#define STEPPER_GPIO_B_N_PIN GPIO_PIN_7 /**< Stepper Motor Coil B Negative. */

#define STEPPER_STEPS_PER_REVOLUTION ((48 * 4) - 4) /**< Number of steps per revolution. */
/**
 * @}
 */

/**
 * \defgroup UART pin definitions.
 * @{
 */
#define UART_TX_GPIO_PIN (GPIO_PIN_6) /**< UART TX Pin. */
#define UART_RX_GPIO_PIN (GPIO_PIN_7) /**< UART RX Pin. */
#define UART_GPIO_PORT   (GPIOB)      /**< UART Port. */
/**
 * @}
 */

/**
 * \defgroup DEBUG pin definitions.
 * @{
 */
#define DEBUG_GPIO_1_PIN (GPIO_PIN_0) /**< DEBUG 1 Pin. */
#define DEBUG_GPIO_2_PIN (GPIO_PIN_1) /**< DEBUG 2 Pin. */
#define DEBUG_GPIO_PORT  (GPIOF)      /**< DEBUG Port. */
/**
 * @}
 */
