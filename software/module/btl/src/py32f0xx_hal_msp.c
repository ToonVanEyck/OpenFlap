/**
 ******************************************************************************
 * @file    py32f0xx_hal_msp.c
 * @author  MCU Application Team
 * @brief   This file provides code for the MSP Initialization
 *          and de-Initialization codes.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) Puya Semiconductor Co.
 * All rights reserved.</center></h2>
 *
 * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "py32f0xx_hal.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static DMA_HandleTypeDef HdmaCh1;
/* Private function prototypes -----------------------------------------------*/
/* External functions --------------------------------------------------------*/

/**
 * @brief  Configure the Flash prefetch and the Instruction cache,
 *         the time base source, NVIC and any required global low level hardware
 *         by calling the HAL_MspInit() callback function from HAL_Init()
 *
 */
void HAL_MspInit(void)
{
}

void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
    __HAL_RCC_GPIOA_CLK_ENABLE(); /* Enable GPIOA clock */
    __HAL_RCC_DMA_CLK_ENABLE();   /* Enable DMA clock */
    __HAL_RCC_ADC_CLK_ENABLE();   /* Enable ADC clock */

    /* ADC channel: PA0 - PA5  */
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin  = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // HAL_NVIC_SetPriority(ADC_COMP_IRQn, 0, 0);
    // HAL_NVIC_EnableIRQ(ADC_COMP_IRQn);
    HAL_SYSCFG_DMA_Req(DMA_CHANNEL_MAP_ADC); /* DMA1_MAP Set to ADC */
    HdmaCh1.Instance                 = DMA1_Channel1;
    HdmaCh1.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    HdmaCh1.Init.PeriphInc           = DMA_PINC_DISABLE;
    HdmaCh1.Init.MemInc              = DMA_MINC_ENABLE;
    HdmaCh1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    HdmaCh1.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    HdmaCh1.Init.Mode                = DMA_CIRCULAR;
    HdmaCh1.Init.Priority            = DMA_PRIORITY_LOW;

    HAL_DMA_DeInit(&HdmaCh1);
    HAL_DMA_Init(&HdmaCh1);
    __HAL_LINKDMA(hadc, DMA_Handle, HdmaCh1);
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef *hadc)
{
    __HAL_RCC_ADC_FORCE_RESET();
    __HAL_RCC_ADC_RELEASE_RESET();
    __HAL_RCC_DMA_FORCE_RESET();
    __HAL_RCC_DMA_RELEASE_RESET();

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5);
    HAL_DMA_DeInit(hadc->DMA_Handle);
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1) {
        __HAL_RCC_TIM1_CLK_ENABLE(); /* Enable TIM clock */
        HAL_NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);
    }
    if (htim->Instance == TIM14) {
        __HAL_RCC_TIM14_CLK_ENABLE(); /* Enable TIM clock */
        HAL_NVIC_SetPriority(TIM14_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM14_IRQn);
    }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1) {
        __HAL_RCC_TIM1_FORCE_RESET();
        __HAL_RCC_TIM1_RELEASE_RESET();
    }
    if (htim->Instance == TIM14) {
        __HAL_RCC_TIM14_FORCE_RESET();
        __HAL_RCC_TIM14_RELEASE_RESET();
    }
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    __HAL_RCC_USART1_CLK_ENABLE(); /* Enable UART clock */

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin       = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF0_USART1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    __HAL_RCC_USART1_FORCE_RESET();
    __HAL_RCC_USART1_RELEASE_RESET();
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6 | GPIO_PIN_7);
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
    __HAL_RCC_TIM3_CLK_ENABLE(); /* Enable TIM clock */

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin       = GPIO_PIN_6;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // HAL_NVIC_SetPriority(TIM3_IRQn, 0, 0); //Not using PWM interrupt
    // HAL_NVIC_EnableIRQ(TIM3_IRQn);
}

void HAL_TIM_PWM_MspDeInit(TIM_HandleTypeDef *htim)
{
    __HAL_RCC_TIM3_FORCE_RESET();
    __HAL_RCC_TIM3_RELEASE_RESET();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_6);
}

void HAL_CRC_MspInit(CRC_HandleTypeDef *hcrc)
{
    __HAL_RCC_CRC_CLK_ENABLE(); /* Enable CRC clock */
}

void HAL_CRC_MspDeInit(CRC_HandleTypeDef *hcrc)
{
    __HAL_RCC_CRC_FORCE_RESET();
    __HAL_RCC_CRC_RELEASE_RESET();
}
/************************ (C) COPYRIGHT Puya *****END OF FILE******************/
