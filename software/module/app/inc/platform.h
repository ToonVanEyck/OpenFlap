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
#define ENCODER_CHANNEL_CNT (3) /**< Number of tracks on the encoder. */

#define ENCODER_CHANNEL_A (0) /**< Encoder Channel A. */
#define ENCODER_CHANNEL_B (2) /**< Encoder Channel B. */
#define ENCODER_CHANNEL_Z (1) /**< Encoder Channel Z. */

/** List of ADC channels used by the encoder. */
#define ENCODER_ADC_CHANNEL_LIST                                                                                       \
    ((uint32_t[ENCODER_CHANNEL_CNT]) {                                                                                 \
        ADC_CHANNEL_3,                                                                                                 \
        ADC_CHANNEL_4,                                                                                                 \
        ADC_CHANNEL_5,                                                                                                 \
    })

#define ENCODER_ADC_GPIO_PINS (GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5) /**< The pins used for ADC by the encoder. */
#define ENCODER_ADC_GPIO_PORT (GPIOA)                                /**< ADC port. */
#define ENCODER_LED_GPIO_PIN  (GPIO_PIN_2) /**< Single pin driving the IR emitters of the IR sensors. */
#define ENCODER_LED_GPIO_PORT (GPIOA)      /**< IR emitter GPIO port. */
/**
 * @}
 */

/**
 * \defgroup Motor pin definitions.
 * @{
 */
#define MOTOR_A_GPIO_PIN  (GPIO_PIN_7) /**< Motor direction pin. */
#define MOTOR_A_GPIO_PORT (GPIOA)      /**< Motor direction port. */
#define MOTOR_B_GPIO_PIN  (GPIO_PIN_6) /**< Motor PMW pin. */
#define MOTOR_B_GPIO_PORT (GPIOA)      /**< Motor PMW port. */
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
