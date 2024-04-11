/* Includes ------------------------------------------------------------------*/
// #include "chain_comm.h"
#include "config.h"
#include "openflap.h"
#include "property_handlers.h"

/* Private define ------------------------------------------------------------*/

/** Convert a miliseconds value into a counter value for the IR/Encoder timer. */
#define IR_TIMER_TICKS_FROM_MS(ms) ((ms) * 10)
/** Convert a microsecond value into a counter value for the IR/Encoder timer. */
#define IR_TIMER_TICKS_FROM_US(us) ((us) / 100)

/** The period of the encoder readings when the system is active. */
#define IR_IDLE_PERIOD_MS IR_TIMER_TICKS_FROM_MS(50)
/** The period of the encoder readings when the system is idle. */
#define IR_ACTIVE_PERIOD_MS IR_TIMER_TICKS_FROM_MS(1000)
/** The IR sensor will iluminate the encoder wheel for this time in microseconds before starting the conversion */
#define IR_ILLUMINATE_TIME_US IR_TIMER_TICKS_FROM_US(200)

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef AdcHandle;
ADC_ChannelConfTypeDef sConfig;
uint32_t aADCxConvertedData[6];

TIM_HandleTypeDef Tim1Handle; // ADC/IR timer

TIM_HandleTypeDef Tim3Handle; // PWM

TIM_HandleTypeDef Tim14Handle; // Oneshot - comms

UART_HandleTypeDef UartHandle;

static openflap_ctx_t openflap_ctx;

static uint8_t uart_rx_buf[1];
static uint8_t uart_tx_buf[1];

static ring_buf_t rx_rb = {.r_cnt = 0, .w_cnt = 0};
static ring_buf_t tx_rb = {.r_cnt = 0, .w_cnt = 0};

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
    HAL_Init();
    BSP_HSI_24MHzClockConfig();
    openflap_ctx.flap_position = SYMBOL_CNT;
    debug_io_init();
    APP_GpioConfig();
    APP_DmaInit();
    APP_AdcConfig();
    APP_UartInit();
    APP_PwmInit();
    APP_OneshotInit();
    APP_TimerInit();

    // HAL_UART_Receive_IT(&UartHandle, uart_rx_buf, 1);

    configLoad(&openflap_ctx.config);
    propertyHandlersInit(&openflap_ctx);

    debug_io_log_info("OpenFlap module has started!\n");

    /* Set setpoint equal to position to prevent instant rotation. */
    while (openflap_ctx.flap_position == SYMBOL_CNT) {
        HAL_Delay(10);
    }
    openflap_ctx.flap_setpoint = openflap_ctx.flap_position;

    uint8_t new_position = 0;
    uint8_t last_tx_rb_used = 0;
    int rtt_key;
    while (1) {

        /* Receive commands from debug_io. */
        rtt_key = debug_io_get();
        if (rtt_key > 0) {
            debug_io_log_debug("received command:  %c\n", (char)rtt_key);
            if (rtt_key == '\n') {
                configPrint(&openflap_ctx.config);
            }
        }

        /* Run chain comm. */
        /* Chain Comm. RX event. */
        if (rb_data_available(&rx_rb) && rb_space_available(&tx_rb)) {
            uint8_t data = rb_data_dequeue(&rx_rb);
            if (chain_comm(&openflap_ctx.chain_ctx, &data, rx_event)) {
                // do {
                rb_data_enqueue(&tx_rb, data);
                last_tx_rb_used = rb_capacity_used(&tx_rb);
                // } while (chain_comm(&openflap_ctx.chain_ctx, &data, tx_event));
            }
        }
        /* Chain Comm. TX event. */
        if (last_tx_rb_used != rb_capacity_used(&tx_rb)) {
            uint8_t data;
            if (chain_comm(&openflap_ctx.chain_ctx, &data, tx_event)) {
                rb_data_enqueue(&tx_rb, data);
                last_tx_rb_used = rb_capacity_used(&tx_rb);
            }
        }
        /* Chain Comm. timeout event. */
        if (chain_comm_timeout) {
            uint8_t data = 0x00;
            chain_comm_timeout = false;
            debug_io_log_debug("Timeout!\n");
            chain_comm(&openflap_ctx.chain_ctx, &data, timeout_event);
        }

        /* Restart Uart RX & TX data if needed. */
        HAL_UART_Receive_IT(&UartHandle, uart_rx_buf, 1);
        if (rb_data_available(&tx_rb)) {
            uart_tx_buf[0] = rb_data_peek(&tx_rb);
            HAL_UART_Transmit_IT(&UartHandle, uart_tx_buf, 1);
        }

        /* Print position. */
        if (new_position != openflap_ctx.flap_position) {
            new_position = openflap_ctx.flap_position;
            debug_io_log_info("Position: %d  %s\n", openflap_ctx.flap_position,
                              &openflap_ctx.config.symbol_set[openflap_ctx.flap_position]);
        }

        /* Set PWM duty cycle. */
        uint8_t distance = flapIndexWrapCalc(SYMBOL_CNT + openflap_ctx.flap_setpoint - openflap_ctx.flap_position);
        __HAL_TIM_SET_COMPARE(&Tim3Handle, TIM_CHANNEL_1, pwmDutyCycleCalc(distance));

        /* Idle logic */
        if (distance == 0 && !rb_data_available(&rx_rb) && !rb_data_available(&tx_rb)) {
            if (!openflap_ctx.is_idle) {
                openflap_ctx.is_idle = true;
                debug_io_log_debug("Idle!\n");
                openflap_ctx.idle_start_ms = HAL_GetTick();
            } else {
                /* Store config if requested and idle for 500ms. */
                if (HAL_GetTick() - openflap_ctx.idle_start_ms > 500 && openflap_ctx.store_config) {
                    openflap_ctx.store_config = false;
                    configStore(&openflap_ctx.config);
                    debug_io_log_info(0, "Config stored!\n");
                }
            }
        } else if (openflap_ctx.is_idle) {
            openflap_ctx.is_idle = false;
            debug_io_log_debug(0, "Active!\n");
        }
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
    AdcHandle.Init.Resolution = ADC_RESOLUTION_10B;           /* 12bit */
    AdcHandle.Init.DataAlign = ADC_DATAALIGN_RIGHT;           /* Right alignment */
    AdcHandle.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD; /* Forward */
    AdcHandle.Init.EOCSelection = ADC_EOC_SEQ_CONV;           /* End flag */
    AdcHandle.Init.LowPowerAutoWait = ENABLE;
    AdcHandle.Init.ContinuousConvMode = DISABLE;
    AdcHandle.Init.DiscontinuousConvMode = DISABLE;
    AdcHandle.Init.ExternalTrigConv = ADC1_2_EXTERNALTRIG_T1_TRGO; /* External trigger: TIM1_TRGO */
    AdcHandle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    AdcHandle.Init.DMAContinuousRequests = ENABLE; /* DMA */
    AdcHandle.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    AdcHandle.Init.SamplingTimeCommon = ADC_SAMPLETIME_41CYCLES_5;
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
    /* (240 * 10) / 24Mhz = 100us */
    Tim1Handle.Instance = TIM1;
    Tim1Handle.Init.Period = 10 - 1;
    Tim1Handle.Init.Prescaler = 240 - 1;
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
    /* (250 * 3) / 8Mhz = 32kHz */
    Tim3Handle.Instance = TIM3;
    Tim3Handle.Init.Period = 255 - 1;
    Tim3Handle.Init.Prescaler = 3;
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
    /* (24000 * 250) / 24Mhz = 250ms */
    Tim14Handle.Instance = TIM14;
    Tim14Handle.Init.Period = 250 - 1;
    Tim14Handle.Init.Prescaler = 24000 - 1;
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
    static uint8_t old_position = SYMBOL_CNT;
    uint8_t encoder_graycode = 0;

    /* Disable IR led. */
    HAL_GPIO_WritePin(GPIO_PORT_LED, GPIO_PIN_LED, GPIO_PIN_RESET);

    // debug_io_log_debug("ADC: %04ld  %04ld  %04ld  %04ld  %04ld  %04ld\n", aADCxConvertedData[IR_MAP[0]],
    //                   aADCxConvertedData[IR_MAP[1]], aADCxConvertedData[IR_MAP[2]], aADCxConvertedData[IR_MAP[3]],
    //                   aADCxConvertedData[IR_MAP[4]], aADCxConvertedData[IR_MAP[5]]);

    /* Convert ADC result into grey code. */
    for (uint8_t i = 0; i < 6; i++) {
        if (aADCxConvertedData[IR_MAP[i]] > openflap_ctx.config.ir_limits[i]) {
            encoder_graycode &= ~(1 << i);
        } else if (aADCxConvertedData[IR_MAP[i]] < openflap_ctx.config.ir_limits[i]) {
            encoder_graycode |= (1 << i);
        }
    }

    /* Stop ADC. */
    if (HAL_ADC_Stop_DMA(&AdcHandle) != HAL_OK) {
        APP_ErrorHandler();
    }

    /* Convert grey code into decimal. */
    uint8_t encoder_decimal = 0;
    for (encoder_decimal = 0; encoder_graycode; encoder_graycode = encoder_graycode >> 1) {
        encoder_decimal ^= encoder_graycode;
    }
    /* Reverse encoder direction. */
    uint8_t new_position = (uint8_t)SYMBOL_CNT - encoder_decimal - 1;

    /* Ignore erroneous reading. */
    if (new_position < SYMBOL_CNT) {
        new_position = flapIndexWrapCalc(new_position + openflap_ctx.config.encoder_offset);
        /* Ignore sensor backspin. */
        if (flapIndexWrapCalc(new_position + 1) != old_position) {
            old_position = new_position;
            openflap_ctx.flap_position = new_position;
        }
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    static uint16_t period_step_cnt = 0xFFFF - 1;

    if (htim->Instance == TIM1) {
        period_step_cnt++;
        /* Start ADC when IR led's have been on for 200us. */
        if (period_step_cnt == IR_ILLUMINATE_TIME_US) {
            if (HAL_ADC_Start_DMA(&AdcHandle, aADCxConvertedData, 6) != HAL_OK) {
                APP_ErrorHandler();
            }

            /* Power IR led's every 50ms. */
        } else if (period_step_cnt >= (openflap_ctx.is_idle ? IR_ACTIVE_PERIOD_MS : IR_IDLE_PERIOD_MS)) {
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
        rb_data_enqueue(&rx_rb, uart_rx_buf[0]);
        HAL_UART_Receive_IT(&UartHandle, uart_rx_buf, 1);
        __HAL_TIM_SET_COUNTER(&Tim14Handle, 0);
        HAL_TIM_Base_Start_IT(&Tim14Handle);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        rb_data_dequeue(&tx_rb);
    }
}

void APP_ErrorHandler(void)
{
    while (1)
        ;
}
