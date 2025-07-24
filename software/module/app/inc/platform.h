#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "debug_io.h"
#include "py32f0xx_bsp_clock.h"

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
