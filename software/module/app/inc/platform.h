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
#define COLEND_GPIO_PIN  GPIO_PIN_12
#define COLEND_GPIO_PORT GPIOA
/**
 * @}
 */

/**
 * \defgroup Absolute encoder IR definitions.
 * @{
 */
#define ENCODER_RESOLUTION (6) /**< Number of tracks on the encoder. */
/** Map sensor adc channel to bit in encoder byte:
 *  - Bit 0 <-- ADC channel 2
 *  - Bit 1 <-- ADC channel 3
 *  - Bit 2 <-- ADC channel 1
 *  - Bit 3 <-- ADC channel 4
 *  - Bit 4 <-- ADC channel 0
 *  - Bit 5 <-- ADC channel 5
 */
#define IR_MAP ((uint8_t[ENCODER_RESOLUTION]) {2, 3, 1, 4, 0, 5})
/** List of ADC channels used by the encoder. */
#define ADC_CHANNEL_LIST                                                                                               \
    ((uint32_t[ENCODER_RESOLUTION]) {                                                                                  \
        ADC_CHANNEL_2,                                                                                                 \
        ADC_CHANNEL_3,                                                                                                 \
        ADC_CHANNEL_1,                                                                                                 \
        ADC_CHANNEL_4,                                                                                                 \
        ADC_CHANNEL_0,                                                                                                 \
        ADC_CHANNEL_5,                                                                                                 \
    })
/** OR of the IR ADC channels */
#define IR_ADC_GPIO_PINS (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5)
#define IR_ADC_GPIO_PORT (GPIOA)      /**< Motor PMW port. */
#define IR_LED_GPIO_PIN  (GPIO_PIN_7) /**< Single pin driving the IR emitters of the IR sensors. */
#define IR_LED_GPIO_PORT (GPIOA)      /**< IR emitter GPIO port. */
/**
 * @}
 */

/**
 * \defgroup Motor pin definitions.
 * @{
 */
#define MOTOR_A_GPIO_PIN  (GPIO_PIN_5) /**< Motor direction pin. */
#define MOTOR_A_GPIO_PORT (GPIOB)      /**< Motor direction port. */
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
