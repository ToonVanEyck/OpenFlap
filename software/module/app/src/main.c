/* Includes ------------------------------------------------------------------*/
// #include "chain_comm.h"
#include "config.h"
#include "openflap.h"
#include "property_handlers.h"
#include "stepper_driver.h"
#include "uart_driver.h"

/* Private define ------------------------------------------------------------*/

/** Convert a milliseconds value into a counter value for the IR/Encoder timer. */
#define IR_TIMER_TICKS_FROM_MS(ms) ((ms) * 10)
/** Convert a microsecond value into a counter value for the IR/Encoder timer. */
#define IR_TIMER_TICKS_FROM_US(us) ((us) / 100)

/** The period of the encoder readings when the motor is idle. */
#define IR_IDLE_PERIOD_MS IR_TIMER_TICKS_FROM_MS(50)
/** The period of the encoder readings when the motor is active. */
#define IR_ACTIVE_PERIOD_MS IR_TIMER_TICKS_FROM_MS(1)
/** The delay between the completion of a step and the start of the IR sensor. */
#define STEPPER_IR_START_DELAY_TICKS IR_TIMER_TICKS_FROM_MS(1)

#define STEPPER_PULE_MIN_MS  (2)  /**< The minimum duration of a stepper pulse. */
#define STEPPER_PULSE_MAX_MS (10) /**< The maximum duration of a stepper pulse. */

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

UART_HandleTypeDef UartHandle;

static openflap_ctx_t openflap_ctx = {0};

#define RB_BUFF_SIZE 128
static uint8_t uart_rx_rb_buff[RB_BUFF_SIZE];
static uint8_t uart_tx_rb_buff[RB_BUFF_SIZE];
static uart_driver_ctx_t uart_driver;
static bool debug_mode        = false;
static bool print_encoder_adc = false;

static uint32_t stepper_rps_x100 = 0;
static bool stepper_enabled      = false;

/* Private macro -------------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
void APP_ErrorHandler(void);
static void APP_GpioConfig(void);
static void APP_TimerInit(void);
// static void APP_PwmInit(void);
static void APP_AdcConfig(void);
static void APP_DmaInit(void);
static void APP_UartInit(void);

static void stepper_set_pins(stepper_driver_ctx_t *ctx, bool a_p, bool a_n, bool b_p, bool b_n);

int main(void)
{
    HAL_Init();
    BSP_HSI_24MHzClockConfig();

    configLoad(&openflap_ctx.config);

    debug_io_init(LOG_LVL_DEBUG);

    APP_GpioConfig();
    APP_DmaInit();
    APP_AdcConfig();
    APP_UartInit();
    // APP_PwmInit();
    APP_TimerInit();

    uart_driver_init(&uart_driver, &UartHandle, uart_rx_rb_buff, RB_BUFF_SIZE, uart_tx_rb_buff, RB_BUFF_SIZE);

    stepper_driver_init(&openflap_ctx.stepper_ctx, 100, STEPPER_STEPS_PER_REVOLUTION, SYMBOL_CNT, stepper_set_pins,
                        NULL);
    stepper_driver_mode_set(&openflap_ctx.stepper_ctx, STEPPER_DRIVER_MODE_FULL_DRIVE);
    stepper_driver_virtual_step_offset_set(&openflap_ctx.stepper_ctx, 0);
    stepper_driver_speed_set(&openflap_ctx.stepper_ctx, 40);
    stepper_driver_virtual_step_offset_set(&openflap_ctx.stepper_ctx, openflap_ctx.config.encoder_offset *
                                                                          STEPPER_STEPS_PER_REVOLUTION / SYMBOL_CNT);

    /* Preform the homing sequence. */
    while (!openflap_ctx.home_found) {
        if (stepper_driver_is_spinning(&openflap_ctx.stepper_ctx)) {
            HAL_Delay(10);
        } else {
            stepper_driver_rotate(&openflap_ctx.stepper_ctx, 2);
        }
    }
    stepper_driver_position_set(&openflap_ctx.stepper_ctx, 0);

    chain_comm_init(&openflap_ctx.chain_ctx, &uart_driver);
    property_handlers_init(&openflap_ctx);

    debug_io_log_info("OpenFlap module has started!\n");
    debug_io_log_info("Version: %s (stepper)\n", GIT_VERSION);
    debug_io_log_info("Compilation Date: %s %s\n", __DATE__, __TIME__);

    volatile int override_key = 0;
    while (1) {

        /* Receive commands from debug_io. */
        int rtt_key = debug_io_get();
        if (override_key > 0) {
            rtt_key      = override_key;
            override_key = 0;
        }
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
                    if (stepper_rps_x100 < 200) {
                        stepper_rps_x100++;
                    }
                    stepper_driver_speed_set(&openflap_ctx.stepper_ctx, stepper_rps_x100);
                    debug_io_log_info("rps: %d,%d\n", stepper_rps_x100 / 100, stepper_rps_x100 % 100);
                    break;
                case '2':
                    if (stepper_rps_x100 > 0) {
                        stepper_rps_x100--;
                    }
                    stepper_driver_speed_set(&openflap_ctx.stepper_ctx, stepper_rps_x100);
                    debug_io_log_info("rps: %d,%d\n", stepper_rps_x100 / 100, stepper_rps_x100 % 100);
                    break;
                case '4':
                    stepper_driver_dir_set(&openflap_ctx.stepper_ctx, STEPPER_DRIVER_DIR_CCW);
                    debug_io_log_info("Direction: CCW\n");
                    break;
                case '6':
                    stepper_driver_dir_set(&openflap_ctx.stepper_ctx, STEPPER_DRIVER_DIR_CW);
                    debug_io_log_info("Direction: CW\n");
                    break;
                case '5':
                    stepper_enabled = !stepper_enabled;
                    stepper_driver_rotate(&openflap_ctx.stepper_ctx,
                                          stepper_enabled ? STEPPER_DRIVER_STEP_INFINITE : 0);
                    debug_io_log_info("%s Stepper\n", stepper_enabled ? "Enabling" : "Disabling");
                    break;
                case '9':
                    stepper_enabled = false;
                    stepper_driver_rotate(&openflap_ctx.stepper_ctx, SYMBOL_CNT);
                    debug_io_log_info("Stepping 1 Revolution\n");
                    break;
                case '3':
                    stepper_enabled = false;
                    stepper_driver_rotate(&openflap_ctx.stepper_ctx, 1);
                    debug_io_log_info("Stepping 1 Revolution\n");
                    break;
            }
        }

        /* Run chain comm. */
        chain_comm(&openflap_ctx.chain_ctx);

        /* Communication status. */
        updateCommsState(&openflap_ctx);

        /* Idle logic. */
        if (!stepper_driver_is_spinning(&openflap_ctx.stepper_ctx) && !openflap_ctx.comms_active) {
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

    /* Configure debug pins, these pins can be used for pin wiggling during development. */
    GPIO_InitStruct.Pin   = DEBUG_GPIO_1_PIN | DEBUG_GPIO_2_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DEBUG_GPIO_PORT, &GPIO_InitStruct);

    /* Configure stepper motor pins. */
    GPIO_InitStruct.Pin   = STEPPER_GPIO_A_P_PIN | STEPPER_GPIO_A_N_PIN | STEPPER_GPIO_B_P_PIN | STEPPER_GPIO_B_N_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(STEPPER_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(STEPPER_GPIO_PORT,
                      STEPPER_GPIO_A_P_PIN | STEPPER_GPIO_A_N_PIN | STEPPER_GPIO_B_P_PIN | STEPPER_GPIO_B_N_PIN,
                      GPIO_PIN_RESET); /* Disable stepper motor. */
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
        debug_io_log_debug("Z: %04ld\n", aADCxConvertedData[ENCODER_CHANNEL_Z]);
    }

    /* Update encoder position. */
    homingPositionDecode(&openflap_ctx, aADCxConvertedData);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) /* 100us timer. */
{
    if (htim->Instance == TIM1) {

        /* Do stepper tick. */
        bool step_complete = stepper_driver_tick(&openflap_ctx.stepper_ctx);
        if (step_complete) {
            openflap_ctx.ir_tick_cnt = STEPPER_IR_START_DELAY_TICKS + IR_ILLUMINATE_TIME_US;
        }

        /* IR sensor. */
        if (openflap_ctx.ir_tick_cnt) {
            openflap_ctx.ir_tick_cnt--;
            if (openflap_ctx.ir_tick_cnt == IR_ILLUMINATE_TIME_US) {
                /* Enable IR led. */
                HAL_GPIO_WritePin(ENCODER_LED_GPIO_PORT, ENCODER_LED_GPIO_PIN, GPIO_PIN_SET);
            } else if (openflap_ctx.ir_tick_cnt == 0) {
                /* Start ADC conversion. */
                if (HAL_ADC_Start_DMA(&AdcHandle, aADCxConvertedData, ENCODER_CHANNEL_CNT) != HAL_OK) {
                    APP_ErrorHandler();
                }
            }
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

/* Callback for the stepper driver. */
void stepper_set_pins(stepper_driver_ctx_t *ctx, bool a_p, bool a_n, bool b_p, bool b_n)
{
    if (a_p == 0 && a_n == 0 && b_p == 0 && b_n == 0) {
        /* Disable stepper motor. */
        HAL_GPIO_WritePin(STEPPER_GPIO_PORT,
                          STEPPER_GPIO_A_P_PIN | STEPPER_GPIO_A_N_PIN | STEPPER_GPIO_B_P_PIN | STEPPER_GPIO_B_N_PIN,
                          GPIO_PIN_RESET);
        return;
    }
    a_p ^= 1;
    a_n ^= 1;
    b_p ^= 1;
    b_n ^= 1;

    HAL_GPIO_WritePin(STEPPER_GPIO_PORT, STEPPER_GPIO_A_P_PIN, a_p);
    HAL_GPIO_WritePin(STEPPER_GPIO_PORT, STEPPER_GPIO_A_N_PIN, a_n);
    HAL_GPIO_WritePin(STEPPER_GPIO_PORT, STEPPER_GPIO_B_P_PIN, b_p);
    HAL_GPIO_WritePin(STEPPER_GPIO_PORT, STEPPER_GPIO_B_N_PIN, b_n);
}