/**
  ******************************************************************************
  * @file    py32f0xx_bsp_led.h
  * @author  MCU Application Team
  * @brief   
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef PY32F003_BSP_LED_H
#define PY32F003_BSP_LED_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "py32f0xx_ll_rcc.h"
#include "py32f0xx_ll_bus.h"
#include "py32f0xx_ll_system.h"
#include "py32f0xx_ll_exti.h"
#include "py32f0xx_ll_cortex.h"
#include "py32f0xx_ll_utils.h"
#include "py32f0xx_ll_pwr.h"
#include "py32f0xx_ll_dma.h"
#include "py32f0xx_ll_gpio.h"
#include "py32f0xx_ll_usart.h"


typedef enum
{
  LED3 = 0,
  LED_GREEN = LED3
} Led_TypeDef;

#define LEDn                               1

#define LED3_PIN                           LL_GPIO_PIN_5
#define LED3_GPIO_PORT                     GPIOB
#define LED3_GPIO_CLK_ENABLE()             LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB)
#define LED3_GPIO_CLK_DISABLE()            LL_IOP_GRP1_DisableClock(LL_IOP_GRP1_PERIPH_GPIOB)

#define LEDx_GPIO_CLK_ENABLE(__INDEX__)    do {LED3_GPIO_CLK_ENABLE(); } while(0U)
#define LEDx_GPIO_CLK_DISABLE(__INDEX__)   LED3_GPIO_CLK_DISABLE()


void             BSP_LED_Init(Led_TypeDef Led);
void             BSP_LED_DeInit(Led_TypeDef Led);
void             BSP_LED_On(Led_TypeDef Led);
void             BSP_LED_Off(Led_TypeDef Led);
void             BSP_LED_Toggle(Led_TypeDef Led);


#ifdef __cplusplus
}
#endif

#endif /* PY32F003_BSP_LED_H */
