/* Includes ------------------------------------------------------------------*/
#include "chain_comm.h"
#include "config.h"
#include "openflap.h"
#include "property_handlers.h"

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef AdcHandle;
ADC_ChannelConfTypeDef sConfig;
uint32_t aADCxConvertedData[6];

TIM_HandleTypeDef Tim1Handle; // ADC/IR timer

TIM_HandleTypeDef Tim3Handle; // PWM

TIM_HandleTypeDef Tim14Handle; // Oneshot - comms

UART_HandleTypeDef UartHandle;

static openflap_ctx_t openflap_ctx;

uint8_t uart_rx_buf[1];

static ring_buf_t rx_rb = {.r_cnt = 0, .w_cnt = 0};

bool chain_comm_timeout = false;

/* Private macro -------------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
void APP_ErrorHandler(void);
static void APP_GpioConfig(void);
static void APP_TimerInit(void);
static void APP_PwmInit(void);
static void APP_OneshotInit(void);
static void APP_AdcConfig(void);
static void APP_DmaInit(void);
static void APP_UartInit(void);

int main(void)
{
    SEGGER_RTT_Init();
    HAL_Init();
    APP_GpioConfig();
    APP_AdcConfig();
    APP_DmaInit();
    APP_UartInit();
    APP_PwmInit();
    APP_OneshotInit();
    APP_TimerInit();

    HAL_UART_Receive_IT(&UartHandle, uart_rx_buf, 1);

    configLoad(&openflap_ctx.config);
    propertyHandlersInit(&openflap_ctx);

    SEGGER_RTT_WriteString(0, "OpenFlap module has started!\r\n");
    uint8_t new_position = 0;
    int rtt_key;
    while (1) {
        /* Receive commands from RTT. */
        rtt_key = SEGGER_RTT_GetKey();
        if (rtt_key > 0) {
            // SEGGER_RTT_printf(0, "\breceived command:  %c\r\n", (char)rtt_key);
            TIM3->CCR1 += 10;
            configPrint(&openflap_ctx.config);
        }

        /* Run chain comm. */
        if (rb_data_available(&rx_rb)) {
            uint8_t data = rb_data_read(&rx_rb);
            if (chain_comm(&openflap_ctx.chain_ctx, &data, rx_event)) {
                do {
                    __HAL_TIM_SET_COUNTER(&Tim14Handle, 0);
                    HAL_TIM_Base_Start_IT(&Tim14Handle);
                    HAL_UART_Transmit(&UartHandle, &data, 1, 100);
                } while (chain_comm(&openflap_ctx.chain_ctx, &data, tx_event));
            }
        }
        if (chain_comm_timeout) {
            uint8_t data = 0x00;
            chain_comm_timeout = false;
            SEGGER_RTT_printf(0, "Timeout!\r\n");
            chain_comm(&openflap_ctx.chain_ctx, &data, timeout_event);
        }

        /* Print position. */
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

    GPIO_InitStruct.Pin = GPIO_PIN_COLEND;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIO_PORT_COLEND, &GPIO_InitStruct);
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

static void APP_OneshotInit(void)
{
    /* (8000 * 250) / 8Mhz = 250ms */
    Tim14Handle.Instance = TIM14;
    Tim14Handle.Init.Period = 250 - 1;
    Tim14Handle.Init.Prescaler = 8000 - 1;
    Tim14Handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    Tim14Handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    Tim14Handle.Init.RepetitionCounter = 0;
    Tim14Handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&Tim14Handle) != HAL_OK) {
        APP_ErrorHandler();
    }
    TIM_MasterConfigTypeDef sMasterConfig;
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&Tim14Handle, &sMasterConfig);
    __HAL_TIM_CLEAR_IT(&Tim14Handle, TIM_IT_UPDATE);
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
        if (aADCxConvertedData[IR_MAP[i]] > openflap_ctx.config.ir_limits[i].ir_high) {
            encoder_graycode &= ~(1 << i);
        } else if (aADCxConvertedData[IR_MAP[i]] < openflap_ctx.config.ir_limits[i].ir_low) {
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
    if (htim->Instance == TIM14) {
        chain_comm_timeout = true;
        HAL_TIM_Base_Stop_IT(&Tim14Handle);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        rx_rb.buf[rx_rb.w_cnt++] = uart_rx_buf[0];
        rx_rb.w_cnt &= 0x0F;
        HAL_UART_Receive_IT(huart, uart_rx_buf, 1);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
    }
}

void APP_ErrorHandler(void)
{
    while (1)
        ;
}
