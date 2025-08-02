/**
 ******************************************************************************
 * @file    py32f0xx_it.c
 * @author  MCU Application Team
 * @brief   Interrupt Service Routines.
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
#include "py32f0xx_it.h"
#include "peripherals.h"

/* Private includes ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private user code ---------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
extern uint32_t systick_1ms_cnt;
extern uint32_t pwm_timer_tick_cnt;
extern peripherals_ctx_t peripherals_ctx;

/******************************************************************************/
/*          Cortex-M0+ Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
 * @brief This function handles Non maskable interrupt.
 */
void NMI_Handler(void)
{
}

/**
 * @brief This function handles Hard fault interrupt.
 */
void HardFault_Handler(void)
{
    while (1) {
    }
}

/**
 * @brief This function handles System service call via SWI instruction.
 */
void SVC_Handler(void)
{
}

/**
 * @brief This function handles Pendable request for system service.
 */
void PendSV_Handler(void)
{
}

/**
 * @brief This function handles System tick timer.
 */
void SysTick_Handler(void)
{
    systick_1ms_cnt++; // Increment the tick count on each update event
}

/******************************************************************************/
/* PY32F0xx Peripheral Interrupt Handlers                                     */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file.                                          */
/******************************************************************************/

void TIM1_BRK_UP_TRG_COM_IRQHandler(void)
{
    /* Clear the TIM1 update interrupt flag */
    if (LL_TIM_IsActiveFlag_UPDATE(TIM1)) {
        LL_TIM_ClearFlag_UPDATE(TIM1);
        LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_4); /* Do interrupt driven PWM for now. */
    }
}

void TIM1_CC_IRQHandler(void)
{
    /* Clear the TIM1 capture/compare interrupt flag */
    if (LL_TIM_IsActiveFlag_CC3(TIM1)) {
        LL_TIM_ClearFlag_CC3(TIM1);
        LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_4); /* Do interrupt driven PWM for now. */
    }
}

void TIM3_IRQHandler(void)
{
    /* Clear the TIM3 UPDATE interrupt flag */
    if (LL_TIM_IsActiveFlag_UPDATE(TIM3)) {
        LL_TIM_ClearFlag_UPDATE(TIM3);
        pwm_timer_tick_cnt++; // Increment the PWM timer tick count
    }
}

// void DMA1_Channel1_IRQHandler(void)
// {
//     /* Clear the transfer complete flag */
//     if (LL_DMA_IsActiveFlag_TC1(DMA1)) {
//         LL_DMA_ClearFlag_TC1(DMA1);
//     }
// }

// void ADC_COMP_IRQHandler(void)
// {
//     /* Clear the ADC1 end-of-conversion flag */
//     if (LL_ADC_IsActiveFlag_EOC(ADC1)) {
//         LL_ADC_ClearFlag_EOC(ADC1);
//     }
// }

void DMA1_Channel2_3_IRQHandler(void)
{
    /* Clear the DMA1 Channel 2 transfer complete flag */
    if (LL_DMA_IsActiveFlag_TC2(DMA1)) {
        LL_DMA_ClearFlag_TC2(DMA1);
        uart_driver_tx_dma_transfer_complete(&peripherals_ctx.uart_driver);
    }

    /* Clear the DMA1 Channel 3 transfer complete flag */
    if (LL_DMA_IsActiveFlag_TC3(DMA1)) {
        LL_DMA_ClearFlag_TC3(DMA1);
    }
}

/************************ (C) COPYRIGHT Puya *****END OF FILE******************/
