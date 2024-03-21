/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

#include "SEGGER_RTT.h"
// #include "py32f0xx_bsp_printf.h"
#include "py32f0xx_hal.h"

#include "config.h"
#include "openflap.h"

/* Private define ------------------------------------------------------------*/
#define GPIO_PIN_LED GPIO_PIN_7
#define GPIO_PORT_LED GPIOA
#define GPIO_PIN_MOTOR GPIO_PIN_6
#define GPIO_PORT_MOTOR GPIOA

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef AdcHandle;
ADC_ChannelConfTypeDef sConfig;
uint32_t aADCxConvertedData[6];

TIM_HandleTypeDef Tim1Handle;

TIM_HandleTypeDef Tim3Handle;

UART_HandleTypeDef UartHandle;

static openflap_config_t openflap_config;
static openflap_ctx_t openflap_ctx;

uint8_t uart_rx_buf[1];
uint8_t uart_tx_buf[1];

/* Private macro -------------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
void APP_ErrorHandler(void);
static void APP_GpioConfig(void);
static void APP_TimerInit(void);
static void APP_PwmInit(void);
static void APP_AdcConfig(void);
static void APP_DmaInit(void);
static void APP_UartInit(void);

int main(void)
{
    HAL_Init();

    APP_GpioConfig();

    APP_TimerInit();

    APP_PwmInit();

    APP_AdcConfig();

    APP_DmaInit();

    APP_UartInit();
    HAL_UART_Receive_IT(&UartHandle, uart_rx_buf, 1);

    configLoad(&openflap_config);

    SEGGER_RTT_WriteString(0, "OpenFlap module has started!\r\n");
    uint8_t new_position = 0xff;
    int rtt_key;
    while (1) {
        rtt_key = SEGGER_RTT_GetKey();
        if (rtt_key > 0) {
            SEGGER_RTT_printf(0, "\breceived command:  %c\r\n", (char)rtt_key);
            uart_tx_buf[0] = (char)rtt_key;
            TIM3->CCR1 += 10;
        }
        if (uart_tx_buf[0]) {
            HAL_UART_Transmit_IT(&UartHandle, uart_tx_buf, 1);
        }
        if (new_position != openflap_ctx.flap_position) {
            new_position = openflap_ctx.flap_position;
            SEGGER_RTT_printf(0, "encoder:  %d\r\n", new_position);
        }
        HAL_Delay(100);
    }
}

static void APP_GpioConfig(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_LED;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIO_PORT_LED, &GPIO_InitStruct);
}

static void APP_AdcConfig(void)
{
    __HAL_RCC_ADC_FORCE_RESET();
    __HAL_RCC_ADC_RELEASE_RESET(); /* Reset ADC */
    __HAL_RCC_ADC_CLK_ENABLE();    /* Enable ADC clock */

    AdcHandle.Instance = ADC1;
    if (HAL_ADCEx_Calibration_Start(&AdcHandle) != HAL_OK) {
        APP_ErrorHandler();
    }
    AdcHandle.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV1; /* ADC clock no division */
    AdcHandle.Init.Resolution = ADC_RESOLUTION_12B;           /* 12bit */
    AdcHandle.Init.DataAlign = ADC_DATAALIGN_RIGHT;           /* Right alignment */
    AdcHandle.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD; /* Backward */
    AdcHandle.Init.EOCSelection = ADC_EOC_SEQ_CONV;           /* End flag */
    AdcHandle.Init.LowPowerAutoWait = ENABLE;
    AdcHandle.Init.ContinuousConvMode = DISABLE;
    AdcHandle.Init.DiscontinuousConvMode = DISABLE;
    AdcHandle.Init.ExternalTrigConv = ADC1_2_EXTERNALTRIG_T1_TRGO;                /* External trigger: TIM1_TRGO */
    AdcHandle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISINGFALLING; /* Triggered by both edges */
    AdcHandle.Init.DMAContinuousRequests = ENABLE;                                /* No DMA */
    AdcHandle.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    AdcHandle.Init.SamplingTimeCommon = ADC_SAMPLETIME_239CYCLES_5;
    if (HAL_ADC_Init(&AdcHandle) != HAL_OK) {
        APP_ErrorHandler();
    }

    sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
    sConfig.Channel = ADC_CHANNEL_0;
    if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK) {
        APP_ErrorHandler();
    }
    sConfig.Channel = ADC_CHANNEL_1;
    if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK) {
        APP_ErrorHandler();
    }
    sConfig.Channel = ADC_CHANNEL_2;
    if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK) {
        APP_ErrorHandler();
    }
    sConfig.Channel = ADC_CHANNEL_3;
    if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK) {
        APP_ErrorHandler();
    }
    sConfig.Channel = ADC_CHANNEL_4;
    if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK) {
        APP_ErrorHandler();
    }
    sConfig.Channel = ADC_CHANNEL_5;
    if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK) {
        APP_ErrorHandler();
    }
}

static void APP_TimerInit(void)
{
    /* (800 * 10) / 8Mhz = 1ms */
    Tim1Handle.Instance = TIM1;
    Tim1Handle.Init.Period = 10 - 1;
    Tim1Handle.Init.Prescaler = 800 - 1;
    Tim1Handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    Tim1Handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    Tim1Handle.Init.RepetitionCounter = 0;
    Tim1Handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&Tim1Handle) != HAL_OK) {
        APP_ErrorHandler();
    }

    TIM_MasterConfigTypeDef sMasterConfig;
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&Tim1Handle, &sMasterConfig);
    if (HAL_TIM_Base_Start_IT(&Tim1Handle) != HAL_OK) {
        APP_ErrorHandler();
    }
}

static void APP_PwmInit(void)
{
    /* (250 * 1) / 8Mhz = 32kHz */
    Tim3Handle.Instance = TIM3;
    Tim3Handle.Init.Period = 255 - 1;
    Tim3Handle.Init.Prescaler = 0;
    Tim3Handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    Tim3Handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    if (HAL_TIM_PWM_Init(&Tim3Handle) != HAL_OK) {
        APP_ErrorHandler();
    }

    TIM_MasterConfigTypeDef sMasterConfig;
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&Tim3Handle, &sMasterConfig);

    TIM_OC_InitTypeDef sConfigOC;
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&Tim3Handle, &sConfigOC, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&Tim3Handle, TIM_CHANNEL_1);
    // HAL_TIM_MspPostInit(&htim4);
}

static void APP_DmaInit(void)
{
    // __HAL_RCC_DMA_CLK_ENABLE();
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
}

static void APP_UartInit(void)
{
    UartHandle.Instance = USART1;
    UartHandle.Init.BaudRate = 115200;
    UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
    UartHandle.Init.StopBits = UART_STOPBITS_1;
    UartHandle.Init.Parity = UART_PARITY_NONE;
    UartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    UartHandle.Init.Mode = UART_MODE_TX_RX;

    if (HAL_UART_Init(&UartHandle) != HAL_OK) {
        APP_ErrorHandler();
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    // SEGGER_RTT_printf(0, "ADC: %04ld  %04ld  %04ld  %04ld  %04ld  %04ld\r\n", aADCxConvertedData[IR_MAP[0]],
    //                   aADCxConvertedData[IR_MAP[1]], aADCxConvertedData[IR_MAP[2]], aADCxConvertedData[IR_MAP[3]],
    //                   aADCxConvertedData[IR_MAP[4]], aADCxConvertedData[IR_MAP[5]]);
    uint8_t encoder_graycode = 0;

    /* Convert ADC result into grey code. */
    for (uint8_t i = 0; i < 6; i++) {
        if (aADCxConvertedData[IR_MAP[i]] > openflap_config.ir_limits[i].ir_high) {
            encoder_graycode &= ~(1 << i);
        } else if (aADCxConvertedData[IR_MAP[i]] < openflap_config.ir_limits[i].ir_low) {
            encoder_graycode |= (1 << i);
        }
    }

    /* Disable IR led's and stop ADC. */
    HAL_GPIO_WritePin(GPIO_PORT_LED, GPIO_PIN_LED, GPIO_PIN_RESET);
    if (HAL_ADC_Stop_DMA(&AdcHandle) != HAL_OK) {
        APP_ErrorHandler();
    }

    /* Convert grey code into decimal. */
    uint8_t encoder_decimal = 0;
    for (encoder_decimal = 0; encoder_graycode; encoder_graycode = encoder_graycode >> 1) {
        encoder_decimal ^= encoder_graycode;
    }
    uint8_t new_position = (uint8_t)SYMBOL_CNT - encoder_decimal - 1;
    if (new_position < SYMBOL_CNT) {
        openflap_ctx.flap_position = new_position;
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    static uint16_t period_step_cnt = 0;

    if (htim->Instance == TIM1) {
        period_step_cnt++;
        /* Start ADC when IR led's have been on for 2ms. */
        if (period_step_cnt == 2) {
            if (HAL_ADC_Start_DMA(&AdcHandle, aADCxConvertedData, 6) != HAL_OK) {
                APP_ErrorHandler();
            }

            /* Power IR led's every 10ms. */
        } else if (period_step_cnt >= 10) {
            period_step_cnt = 0;
            HAL_GPIO_WritePin(GPIO_PORT_LED, GPIO_PIN_LED, GPIO_PIN_SET);
        }
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        uart_tx_buf[0] = uart_rx_buf[0];
        SEGGER_RTT_printf(0, "uart RX:  %c\r\n", uart_rx_buf[0]);
        HAL_UART_Receive_IT(huart, uart_rx_buf, 1);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        SEGGER_RTT_printf(0, "uart TX:  %c\r\n", uart_tx_buf[0]);
        uart_tx_buf[0] = 0;
    }
}

void APP_ErrorHandler(void)
{
    while (1)
        ;
}
