/**
 ******************************************************************************
 * @file    py32f0xx_bsp_printf.h
 * @author  MCU Application Team
 * @brief
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef PY32F0XX_BSP_PRINTF_H
#define PY32F0XX_BSP_PRINTF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "py32f0xx_hal.h"
#include <stdio.h>

#ifdef HAL_UART_MODULE_ENABLED
// debug printf redirect config
#define DEBUG_USART_BAUDRATE 115200

#define DEBUG_USART USART2

#define DEBUG_USART_CLK_ENABLE()                                                                                       \
    do {                                                                                                               \
        __IO uint32_t tmpreg = 0x00U;                                                                                  \
        SET_BIT(RCC->APBENR1, RCC_APBENR1_USART2EN);                                                                   \
        /* Delay after an RCC peripheral clock enabling */                                                             \
        tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_USART2EN);                                                         \
        UNUSED(tmpreg);                                                                                                \
    } while (0U)

#define __GPIOF_CLK_ENABLE()                                                                                           \
    do {                                                                                                               \
        __IO uint32_t tmpreg = 0x00U;                                                                                  \
        SET_BIT(RCC->IOPENR, RCC_IOPENR_GPIOFEN);                                                                      \
        /* Delay after an RCC peripheral clock enabling */                                                             \
        tmpreg = READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIOFEN);                                                            \
        UNUSED(tmpreg);                                                                                                \
    } while (0U)

#define DEBUG_USART_RX_GPIO_PORT GPIOF
#define DEBUG_USART_RX_GPIO_CLK_ENABLE() __HAL_RCC_GPIOF_CLK_ENABLE()
#define DEBUG_USART_RX_PIN GPIO_PIN_1
#define DEBUG_USART_RX_AF GPIO_AF9_USART2

#define DEBUG_USART_TX_GPIO_PORT GPIOF
#define DEBUG_USART_TX_GPIO_CLK_ENABLE() __HAL_RCC_GPIOF_CLK_ENABLE()
#define DEBUG_USART_TX_PIN GPIO_PIN_0
#define DEBUG_USART_TX_AF GPIO_AF9_USART2

#define DEBUG_USART_IRQHandler USART2_IRQHandler
#define DEBUG_USART_IRQ USART2_IRQn

extern UART_HandleTypeDef DebugUartHandle;
#endif

void BSP_USART_Config(void);

#ifdef __cplusplus
}
#endif

#endif /* PY32F0XX_BSP_PRINTF_H */
