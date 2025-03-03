/* Includes ------------------------------------------------------------------*/
// #include "chain_comm.h"
#include "config.h"
#include "openflap.h"
#include "property_handlers.h"
#include "uart_driver.h"

/* Private define ------------------------------------------------------------*/

/** Convert a milliseconds value into a counter value for the IR/Encoder timer. */
#define IR_TIMER_TICKS_FROM_MS(ms) ((ms) * 10)
/** Convert a microsecond value into a counter value for the IR/Encoder timer. */
#define IR_TIMER_TICKS_FROM_US(us) ((us) / 100)

/** The period of the encoder readings when the system is idle. */
#define IR_IDLE_PERIOD_MS IR_TIMER_TICKS_FROM_MS(1000)
/** The period of the encoder readings when the system is active and the distance to the setpoint flap is large. */
#define IR_ACTIVE_DISTANCE_LARGE_PERIOD_MS IR_TIMER_TICKS_FROM_MS(10)
/** The period of the encoder readings when the system is active and the distance to the setpoint flap is small. */
#define IR_ACTIVE_DISTANCE_SMALL_PERIOD_MS IR_TIMER_TICKS_FROM_MS(5)
/** The period of the encoder readings when the system is active and the distance to the setpoint flap is very small. */
#define IR_ACTIVE_DISTANCE_VERY_SMALL_PERIOD_MS IR_TIMER_TICKS_FROM_MS(3)

/** The IR sensor will illuminate the encoder wheel for this time in microseconds before starting the conversion */
#define IR_ILLUMINATE_TIME_US IR_TIMER_TICKS_FROM_US(200)

#ifndef VERSION
#define VERSION "not found"
#endif

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef AdcHandle;
ADC_ChannelConfTypeDef sConfig;
uint32_t aADCxConvertedData[ENCODER_CHANNEL_CNT] = {0};

TIM_HandleTypeDef Tim1Handle; // ADC/IR timer

TIM_HandleTypeDef motorPwmHandle; // PWM

UART_HandleTypeDef UartHandle;

static openflap_ctx_t openflap_ctx = {0};

#define RB_BUFF_SIZE 128
static uint8_t uart_rx_rb_buff[RB_BUFF_SIZE];
static uint8_t uart_tx_rb_buff[RB_BUFF_SIZE];
static uart_driver_ctx_t uart_driver;
static bool debug_mode        = false;
static bool print_encoder_adc = false;

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
    BSP_HSI_24MHzClockConfig();

    configLoad(&openflap_ctx.config);
    openflap_ctx.flap_position = SYMBOL_CNT;

    debug_io_init(LOG_LVL_DEBUG);

    APP_GpioConfig();
    APP_DmaInit();
    APP_AdcConfig();
    APP_UartInit();
    APP_PwmInit();
    APP_TimerInit();

    uart_driver_init(&uart_driver, &UartHandle, uart_rx_rb_buff, RB_BUFF_SIZE, uart_tx_rb_buff, RB_BUFF_SIZE);

    chain_comm_init(&openflap_ctx.chain_ctx, &uart_driver);
    property_handlers_init(&openflap_ctx);

    debug_io_log_info("OpenFlap module has started!\n");
    debug_io_log_info("Version: %s\n", GIT_VERSION);
    debug_io_log_info("Compilation Date: %s %s\n", __DATE__, __TIME__);

    /* Set setpoint equal to position to prevent instant rotation. */
    // while (openflap_ctx.flap_position == SYMBOL_CNT) {
    //     HAL_Delay(10);
    // }
    // openflap_ctx.flap_setpoint = openflap_ctx.flap_position;

    uint8_t new_position = 0;
    int rtt_key;
    int16_t speed = 0;
    while (1) {

        /* Receive commands from debug_io. */
        rtt_key = debug_io_get();
        if (rtt_key > 0) {
            debug_io_log_debug("received command:  %c\n", (char)rtt_key);
            switch (rtt_key) {
                case '\n':
                    configPrint(&openflap_ctx.config);
                    break;
                case 'a':
                    print_encoder_adc = !print_encoder_adc;
                    debug_io_log_info("%s Encoder ADC\n", print_encoder_adc ? "Printing" : "Not printing");
                    break;
                case 'd':
                    debug_mode = !debug_mode;
                    debug_io_log_info("%s Debug Mode\n", debug_mode ? "Entering" : "Exiting");
                    break;
                case '8':
                    speed += 5;
                    debug_io_log_info("Speed: %d\n", speed);
                    break;
                case '2':
                    speed -= 5;
                    debug_io_log_info("Speed: %d\n", speed);
                    break;
            }
        }

        /* Run chain comm. */
        chain_comm(&openflap_ctx.chain_ctx);

        /* Set debug pins based on flap position. */
        HAL_GPIO_WritePin(DEBUG_GPIO_PORT, DEBUG_GPIO_1_PIN, openflap_ctx.flap_position & 1);
        HAL_GPIO_WritePin(DEBUG_GPIO_PORT, DEBUG_GPIO_2_PIN, openflap_ctx.flap_position == 0);

        /* Print position. */
        if (new_position != openflap_ctx.flap_position) {
            new_position                              = openflap_ctx.flap_position;
            static uint32_t last_position_change_time = 0;
            uint32_t current_time                     = HAL_GetTick();
            uint32_t time_since_last_change           = current_time - last_position_change_time;
            last_position_change_time                 = current_time;
            debug_io_log_info("Pos: %d  %s (%ld ms)\n", openflap_ctx.flap_position,
                              &openflap_ctx.config.symbol_set[openflap_ctx.flap_position], time_since_last_change);
        }

        if (!debug_mode) {
            /* Set the motor speed based on the distance between the current and target flap. */
            setMotorFromDistance(&openflap_ctx);
            // motorIdle();
        } else {
            if (speed < 0) {
                motorReverse(-speed);
            } else if (speed > 0) {
                motorForward(speed);
            } else {
                motorBrake();
            }
        }

        /* Communication status. */
        updateCommsState(&openflap_ctx);

        /* Motor status. */
        updateMotorState(&openflap_ctx);

        /* Idle logic. */
        if (!openflap_ctx.motor_active && !openflap_ctx.comms_active) {
            if (openflap_ctx.store_config) {
                openflap_ctx.store_config = false;
                configStore(&openflap_ctx.config);
                debug_io_log_info(0, "Config stored!\n");
            }
            if (openflap_ctx.reboot) {
                debug_io_log_info(0, "Rebooting module!\n");
                NVIC_SystemReset();
            }
        }
    }
}

static void APP_GpioConfig(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();

    /* Configure column-end detection pin. */
    GPIO_InitStruct.Pin   = COLEND_GPIO_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(COLEND_GPIO_PORT, &GPIO_InitStruct);

    /* Configure IR LED */
    GPIO_InitStruct.Pin   = ENCODER_LED_GPIO_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(ENCODER_LED_GPIO_PORT, &GPIO_InitStruct);

    /* Configure MOTOR A output. (motor direction) */
    GPIO_InitStruct.Pin   = MOTOR_A_GPIO_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(MOTOR_A_GPIO_PORT, &GPIO_InitStruct);

    /* Configure debug pins, these pins can be used for pin wiggling during development. */
    GPIO_InitStruct.Pin   = DEBUG_GPIO_1_PIN | DEBUG_GPIO_2_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DEBUG_GPIO_PORT, &GPIO_InitStruct);
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
    AdcHandle.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV1;   /* ADC clock no division */
    AdcHandle.Init.Resolution            = ADC_RESOLUTION_10B;         /* 12bit */
    AdcHandle.Init.DataAlign             = ADC_DATAALIGN_RIGHT;        /* Right alignment */
    AdcHandle.Init.ScanConvMode          = ADC_SCAN_DIRECTION_FORWARD; /* Forward */
    AdcHandle.Init.EOCSelection          = ADC_EOC_SEQ_CONV;           /* End flag */
    AdcHandle.Init.LowPowerAutoWait      = ENABLE;
    AdcHandle.Init.ContinuousConvMode    = DISABLE;
    AdcHandle.Init.DiscontinuousConvMode = DISABLE;
    AdcHandle.Init.ExternalTrigConv      = ADC1_2_EXTERNALTRIG_T1_TRGO; /* External trigger: TIM1_TRGO */
    AdcHandle.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    AdcHandle.Init.DMAContinuousRequests = ENABLE; /* DMA */
    AdcHandle.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;
    AdcHandle.Init.SamplingTimeCommon    = ADC_SAMPLETIME_41CYCLES_5;
    if (HAL_ADC_Init(&AdcHandle) != HAL_OK) {
        APP_ErrorHandler();
    }

    sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
    /* Configure the ADC channels. */
    for (uint8_t i = 0; i < ENCODER_CHANNEL_CNT; i++) {
        sConfig.Channel = ENCODER_ADC_CHANNEL_LIST[i];
        if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK) {
            APP_ErrorHandler();
        }
    }
}

static void APP_TimerInit(void)
{
    /* (240 * 10) / 24Mhz = 100us */
    Tim1Handle.Instance               = TIM1;
    Tim1Handle.Init.Period            = 10 - 1;
    Tim1Handle.Init.Prescaler         = 240 - 1;
    Tim1Handle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    Tim1Handle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    Tim1Handle.Init.RepetitionCounter = 0;
    Tim1Handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&Tim1Handle) != HAL_OK) {
        APP_ErrorHandler();
    }

    TIM_MasterConfigTypeDef sMasterConfig;
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&Tim1Handle, &sMasterConfig);
    if (HAL_TIM_Base_Start_IT(&Tim1Handle) != HAL_OK) {
        APP_ErrorHandler();
    }
}

static void APP_PwmInit(void)
{
    /* (250 * 3) / 8Mhz = 32kHz */
    motorPwmHandle.Instance           = TIM3;
    motorPwmHandle.Init.Period        = 255 - 1;
    motorPwmHandle.Init.Prescaler     = 3;
    motorPwmHandle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    motorPwmHandle.Init.CounterMode   = TIM_COUNTERMODE_UP;
    if (HAL_TIM_PWM_Init(&motorPwmHandle) != HAL_OK) {
        APP_ErrorHandler();
    }

    TIM_MasterConfigTypeDef sMasterConfig;
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&motorPwmHandle, &sMasterConfig);

    TIM_OC_InitTypeDef sConfigOC;
    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
    sConfigOC.Pulse      = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&motorPwmHandle, &sConfigOC, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&motorPwmHandle, TIM_CHANNEL_1);
}

static void APP_DmaInit(void)
{
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
}

static void APP_UartInit(void)
{
    UartHandle.Instance        = USART1;
    UartHandle.Init.BaudRate   = 115200;
    UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
    UartHandle.Init.StopBits   = UART_STOPBITS_1;
    UartHandle.Init.Parity     = UART_PARITY_NONE;
    UartHandle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    UartHandle.Init.Mode       = UART_MODE_TX_RX;

    if (HAL_UART_Init(&UartHandle) != HAL_OK) {
        APP_ErrorHandler();
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    /* Disable IR led. */
    HAL_GPIO_WritePin(ENCODER_LED_GPIO_PORT, ENCODER_LED_GPIO_PIN, GPIO_PIN_RESET);

    if (print_encoder_adc) {
        debug_io_log_debug("ABZ: %04ld %04ld %04ld\n", aADCxConvertedData[ENCODER_CHANNEL_A],
                           aADCxConvertedData[ENCODER_CHANNEL_B], aADCxConvertedData[ENCODER_CHANNEL_Z]);
    }

    /* Update encoder position. */
    encoderPositionUpdate(&openflap_ctx, aADCxConvertedData);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1) {
        openflap_ctx.ir_tick_cnt++;
        uint16_t ir_period = IR_IDLE_PERIOD_MS;
        if (openflap_ctx.motor_active) {
            if (openflap_ctx.flap_distance == 1) {
                ir_period = IR_ACTIVE_DISTANCE_VERY_SMALL_PERIOD_MS;
            } else if (openflap_ctx.flap_distance <= 3) {
                ir_period = IR_ACTIVE_DISTANCE_SMALL_PERIOD_MS;
            } else {
                ir_period = IR_ACTIVE_DISTANCE_LARGE_PERIOD_MS;
            }
        }
        ir_period = IR_ACTIVE_DISTANCE_VERY_SMALL_PERIOD_MS;
        /* Start ADC when IR led's have been on for 200us. */
        if (openflap_ctx.ir_tick_cnt == IR_ILLUMINATE_TIME_US) {
            if (HAL_ADC_Start_DMA(&AdcHandle, aADCxConvertedData, ENCODER_CHANNEL_CNT) != HAL_OK) {
                APP_ErrorHandler();
            }

            /* Power IR led's. */
        } else if (openflap_ctx.ir_tick_cnt >= ir_period) {
            openflap_ctx.ir_tick_cnt = 0;
            HAL_GPIO_WritePin(ENCODER_LED_GPIO_PORT, ENCODER_LED_GPIO_PIN, GPIO_PIN_SET);
        }
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        if (huart->ErrorCode == HAL_UART_ERROR_NONE) {
            uart_driver_rx_isr(&uart_driver);
        }
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        uart_driver_ctx_tx_isr(&uart_driver);
    }
}

void APP_ErrorHandler(void)
{
    while (1)
        ;
}
