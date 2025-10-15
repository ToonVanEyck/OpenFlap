/**
 * @file openflap_hal_cct.c
 * Openflap hardware abstraction layer.
 */

#include "openflap_hal.h"
#include "flash.h"
#include "rbuff.h"
#include "uart_driver.h"

//======================================================================================================================
//                                                  DEFINES AND CONSTS
//======================================================================================================================

// /** Number of debug GPIO pins. */
// #define DEBUG_GPIO_PINS_COUNT (0)
// /** Debug GPIO pins. */
// const uint32_t DEBUG_GPIO_PINS[DEBUG_GPIO_PINS_COUNT] = {LL_GPIO_PIN_0};
// /** Debug GPIO ports. */
// GPIO_TypeDef *const DEBUG_GPIO_PORTS[DEBUG_GPIO_PINS_COUNT] = {GPIOF};

/** Flash memory offset for config. */
extern uint32_t __FLASH_NVS_START__;
#define NVS_START_ADDR ((uint32_t) & __FLASH_NVS_START__)

//======================================================================================================================
//                                                   GLOBAL VARIABLES
//======================================================================================================================

uint32_t systick_1ms_cnt       = 0;    /**< Counter for SysTick events. */
uint32_t pwm_timer_tick_cnt    = 0;    /**< Counter for TIM3 update events. */
uint32_t sens_timer_tick_cnt   = 0;    /**< Counter for TIM1 update events. */
uart_driver_ctx_t *uart_driver = NULL; /**< Reference to uart driver to be used by interrupt handlers. */

/* ADC Input Capture DMA Buffer */
volatile uint32_t adc_dma_buf[ENCODER_CHANNEL_COUNT] = {0};

/* UART DMA buffers */
#define UART_DMA_BUF_LEN 64
volatile uint8_t uart_rx_dma_buf[UART_DMA_BUF_LEN] = {0}; /**< UART RX DMA ring buffer */
uint8_t uart_tx_buffer[UART_DMA_BUF_LEN]           = {0}; /**< UART TX buffer for ring buffer */
volatile uint8_t uart_tx_dma_buf[UART_DMA_BUF_LEN] = {0}; /**< UART TX DMA buffer */

//======================================================================================================================
//                                                  FUNCTION PROTOTYPES
//======================================================================================================================

static void of_hal_gpio_init(void);  /**< GPIO initialization. */
static void of_hal_tim1_init(void);  /**< TIM1 initialization. (IR LED / Master) */
static void of_hal_tim3_init(void);  /**< TIM3 initialization. (Motor PWM / Slave) */
static void of_hal_adc1_init(void);  /**< ADC1 initialization. (IR sensors) */
static void of_hal_uart1_init(void); /**< UART1 initialization. (chain comm) */

static void *of_hal_uart_dma_w_ptr_get(void);        /**< Get the write pointer for UART DMA buffer. */
static void of_hal_uart_tx_dma_start(size_t length); /**< Start a DMA transfer for UART TX. */

//======================================================================================================================
//                                                   PUBLIC FUNCTIONS
//======================================================================================================================

void of_hal_init(of_hal_ctx_t *of_hal_ctx)
{
    BSP_RCC_HSI_24MConfig();

    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);

    LL_SYSTICK_EnableIT(); /* Enable SysTick interrupt. */

    uart_driver_init(&of_hal_ctx->uart_driver, uart_rx_dma_buf, UART_DMA_BUF_LEN, uart_tx_buffer, UART_DMA_BUF_LEN,
                     uart_tx_dma_buf, UART_DMA_BUF_LEN, of_hal_uart_dma_w_ptr_get, of_hal_uart_tx_dma_start);
    uart_driver = &of_hal_ctx->uart_driver; /* Store reference for interrupt use. */

    of_hal_gpio_init();
    of_hal_tim1_init();
    of_hal_tim3_init();
    of_hal_adc1_init();
    of_hal_uart1_init();
}

//----------------------------------------------------------------------------------------------------------------------

void of_hal_deinit(void)
{
}

//-----------------------------------------------------------------------------------------------------------------------

uint32_t of_hal_tick_count_get(void)
{
    return systick_1ms_cnt;
}

//----------------------------------------------------------------------------------------------------------------------

uint32_t of_hal_pwm_tick_count_get(void)
{
    return pwm_timer_tick_cnt;
}

//----------------------------------------------------------------------------------------------------------------------

uint32_t of_hal_sens_tick_count_get(void)
{
    return sens_timer_tick_cnt;
}

//------------------------------------------------------------------------------------------------------------------------

void of_hal_debug_pin_set(uint8_t pin, bool value)
{
    // if (pin >= DEBUG_GPIO_PINS_COUNT) {
    //     return; // Invalid pin number
    // }
    // if (value) {
    //     LL_GPIO_SetOutputPin(DEBUG_GPIO_PORTS[pin], DEBUG_GPIO_PINS[pin]);
    // } else {
    //     LL_GPIO_ResetOutputPin(DEBUG_GPIO_PORTS[pin], DEBUG_GPIO_PINS[pin]);
    // }
}

//----------------------------------------------------------------------------------------------------------------------

void of_hal_debug_pin_toggle(uint8_t pin)
{
    // if (pin >= DEBUG_GPIO_PINS_COUNT) {
    //     return; // Invalid pin number
    // }
    // LL_GPIO_TogglePin(DEBUG_GPIO_PORTS[pin], DEBUG_GPIO_PINS[pin]);
}

//----------------------------------------------------------------------------------------------------------------------

bool of_hal_debug_pin_get(uint8_t pin)
{
    // if (pin >= DEBUG_GPIO_PINS_COUNT) {
    //     return false; // Invalid pin number
    // }
    // return LL_GPIO_IsOutputPinSet(DEBUG_GPIO_PORTS[pin], DEBUG_GPIO_PINS[pin]);
    return false;
}

//----------------------------------------------------------------------------------------------------------------------

void of_hal_led_set(bool value)
{
    if (value) {
        LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_7);
    } else {
        LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_7);
    }
}

//----------------------------------------------------------------------------------------------------------------------

void of_hal_led_toggle()
{
    LL_GPIO_TogglePin(GPIOB, LL_GPIO_PIN_7);
}

//----------------------------------------------------------------------------------------------------------------------

void of_hal_motor_control(int16_t speed, int16_t decay)
{
    /* Clamp input values */
    speed = (speed < -1000) ? -1000 : (speed > 1000) ? 1000 : speed; // Clamp speed to [-1000, 1000]
    decay = (decay < 0) ? 0 : (decay > 1000) ? 1000 : decay;         // Clamp decay to [0, 1000]

    /* Convert speed to pwm, scalar might be needed for different timer configurations. */
    uint16_t pwm_value = (speed >= 0) ? speed : -speed;

    /* Select correct register based on direction. */
    volatile uint32_t *pwm_reg_1 = (speed >= 0) ? &TIM1->CCR4 : &TIM1->CCR3;
    volatile uint32_t *pwm_reg_2 = (speed >= 0) ? &TIM1->CCR3 : &TIM1->CCR4;

    /* Calculate pwm values based on speed and decay. */
    uint16_t pwm_1 = (uint32_t)(1000 - pwm_value) * decay / 1000;
    uint16_t pwm_2 = (uint32_t)(pwm_value) * (1000 - decay) / 1000 + decay;

    /* Write new PWM values to registers. */
    WRITE_REG(*pwm_reg_1, pwm_1);
    WRITE_REG(*pwm_reg_2, pwm_2);
}

//----------------------------------------------------------------------------------------------------------------------

bool of_hal_motor_is_running(void)
{
    return (LL_TIM_OC_GetCompareCH3(TIM1) != LL_TIM_OC_GetCompareCH4(TIM1));
}

//----------------------------------------------------------------------------------------------------------------------

bool of_hal_is_column_end(void)
{
    return LL_GPIO_IsInputPinSet(GPIOA, LL_GPIO_PIN_12);
}

//----------------------------------------------------------------------------------------------------------------------

void of_hal_encoder_values_get(uint16_t *encoder_values)
{
    // Map the dma buffer to the encoder channels.
    encoder_values[ENC_CH_A] = adc_dma_buf[0]; // Encoder channel A
    encoder_values[ENC_CH_B] = adc_dma_buf[1]; // Encoder channel B
    encoder_values[ENC_CH_Z] = adc_dma_buf[2]; // Encoder channel Z
}

//----------------------------------------------------------------------------------------------------------------------

void of_hal_uart_tx_pin_update(bool enable_secondary_tx)
{
    static bool secondary_tx_enabled = false;

    if (enable_secondary_tx != secondary_tx_enabled) {
        secondary_tx_enabled = enable_secondary_tx;
        LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_2, enable_secondary_tx ? LL_GPIO_MODE_ALTERNATE : LL_GPIO_MODE_INPUT);
    }
}

//----------------------------------------------------------------------------------------------------------------------

void of_hal_config_store(of_config_t *config)
{
    flash_write(NVS_START_ADDR, (uint8_t *)config, sizeof(of_config_t));
}

//----------------------------------------------------------------------------------------------------------------------

void of_hal_config_load(of_config_t *config)
{
    flash_read(NVS_START_ADDR, (uint8_t *)config, sizeof(of_config_t));
}

//----------------------------------------------------------------------------------------------------------------------

void of_hal_ir_timer_idle_set(bool idle)
{
    if (idle) {
        LL_TIM_SetPrescaler(TIM3, (24 * 5) - 1);         /* Reduce freq by factor 5 */
        LL_TIM_OC_SetCompareCH3(TIM3, 1000 - (220 / 5)); /* IR LED */
        LL_TIM_OC_SetCompareCH4(TIM3, 1000 - (20 / 5));  /* ADC trigger */
    } else {
        LL_TIM_SetPrescaler(TIM3, 24 - 1);
        LL_TIM_OC_SetCompareCH3(TIM3, 1000 - 220); /* IR LED */
        LL_TIM_OC_SetCompareCH4(TIM3, 1000 - 20);  /* ADC trigger */
    }
}

//======================================================================================================================
//                                                         PRIVATE FUNCTIONS
//======================================================================================================================

/**
 * @brief Initializes GPIOs used for debugging.
 *
 * Configured pins:
 * - PA12: Column end detection (input)
 * - PB5: Power OK detection (input)
 * - PF0: Debug pin 0 (output)
 */
static void of_hal_gpio_init(void)
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable GPIO clocks. */
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOF);

    /* Initialize the "column-end" detection pin. (PA12) */
    GPIO_InitStruct.Pin   = LL_GPIO_PIN_12;
    GPIO_InitStruct.Mode  = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Initialize the "12V-ok" detection pin. (PB5) */
    GPIO_InitStruct.Pin   = LL_GPIO_PIN_5;
    GPIO_InitStruct.Mode  = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Initialize the status LED. (PB7) */
    GPIO_InitStruct.Pin        = LL_GPIO_PIN_7;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_HIGH;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_7); // LED off

    /* Initialize the 1 debug pins. */
    // GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    // GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    // GPIO_InitStruct.Pull       = LL_GPIO_PULL_UP;
    // GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_HIGH;
    // for (int i = 0; i < DEBUG_GPIO_PINS_COUNT; i++) {
    //     GPIO_InitStruct.Pin = DEBUG_GPIO_PINS[i];
    //     LL_GPIO_Init(DEBUG_GPIO_PORTS[i], &GPIO_InitStruct);
    // }
}

//-----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Initializes TIM1 for PWM on channel 3 and 4.
 * TIM1 is configured as a slave at 333.33Hz.
 */
static void of_hal_tim1_init(void)
{

    // Enable TIM1 Clock
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_TIM1);

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode                = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed               = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pull                = LL_GPIO_PULL_DOWN;

    // Configure PA0 (TIM1_CH3) and PA1 (TIM1_CH4) as alternate function for PWM.
    GPIO_InitStruct.Pin       = LL_GPIO_PIN_0 | LL_GPIO_PIN_1;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_13; // TIM1_CH3 & TIM1_CH4
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Configure TIM1 base: 24MHz / (1000 * 120) ≈ 333.33Hz (1000 counts of 3us)
    LL_TIM_SetPrescaler(TIM1, 72 - 1);
    LL_TIM_SetAutoReload(TIM1, 1000 - 1);
    LL_TIM_SetCounterMode(TIM1, LL_TIM_COUNTERMODE_UP);

    // Configure PWM mode for Channel 3 and 4 (motor)
    LL_TIM_OC_SetMode(TIM1, LL_TIM_CHANNEL_CH3, LL_TIM_OCMODE_PWM1);
    LL_TIM_OC_SetPolarity(TIM1, LL_TIM_CHANNEL_CH3, LL_TIM_OCPOLARITY_HIGH);
    LL_TIM_OC_SetCompareCH3(TIM1, 0);
    LL_TIM_OC_EnablePreload(TIM1, LL_TIM_CHANNEL_CH3);
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH3);

    LL_TIM_OC_SetMode(TIM1, LL_TIM_CHANNEL_CH4, LL_TIM_OCMODE_PWM1);
    LL_TIM_OC_SetPolarity(TIM1, LL_TIM_CHANNEL_CH4, LL_TIM_OCPOLARITY_HIGH);
    LL_TIM_OC_SetCompareCH4(TIM1, 0);
    LL_TIM_OC_EnablePreload(TIM1, LL_TIM_CHANNEL_CH4);
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH4);

    LL_TIM_EnableAllOutputs(TIM1);

    // Configure TIM1 as master - trigger output on update event
    LL_TIM_SetTriggerOutput(TIM1, LL_TIM_TRGO_UPDATE);

    // Enable interrupt for TIM1 Update
    LL_TIM_EnableIT_UPDATE(TIM1);
    NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, 2);
    NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);

    // Enable the timer (master timer starts first)
    LL_TIM_EnableCounter(TIM1);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Initializes TIM3 for IR LED control.
 * TIMER 3 is configure as master at 1kHz.
 */

static void of_hal_tim3_init(void)
{
    // Enable TIM1 Clock
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);

    // Configure PA4 as output.
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode                = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed               = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pull                = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Pin                 = LL_GPIO_PIN_4;
    GPIO_InitStruct.Alternate           = LL_GPIO_AF_13; // TIM3_CH3
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Configure TIM3 base: 24MHz / (1000 * 24) ≈ 1000Hz (1000 counts of 1us)
    LL_TIM_SetPrescaler(TIM3, 24 - 1);
    LL_TIM_SetAutoReload(TIM3, 1000 - 1);
    LL_TIM_SetCounterMode(TIM3, LL_TIM_COUNTERMODE_UP);

    // Configure PWM mode for Channel 3 (IR LED)
    LL_TIM_OC_SetMode(TIM3, LL_TIM_CHANNEL_CH3, LL_TIM_OCMODE_PWM1);
    LL_TIM_OC_SetPolarity(TIM3, LL_TIM_CHANNEL_CH3, LL_TIM_OCPOLARITY_LOW);
    LL_TIM_OC_SetCompareCH3(TIM3, 1000 - 220); /* IR LED should be on for 220us each cycle. */
    LL_TIM_OC_EnablePreload(TIM3, LL_TIM_CHANNEL_CH3);
    LL_TIM_CC_EnableChannel(TIM3, LL_TIM_CHANNEL_CH3);

    // Configure Channel 4 as output compare for external trigger (ADC trigger) Not connected to physical pin.
    LL_TIM_OC_SetMode(TIM3, LL_TIM_CHANNEL_CH4, LL_TIM_OCMODE_PWM2); /* Generates a rising edge after 200us. */
    LL_TIM_OC_SetPolarity(TIM3, LL_TIM_CHANNEL_CH4, LL_TIM_OCPOLARITY_LOW);
    LL_TIM_OC_SetCompareCH4(TIM3, 1000 - 20); /* ADC should start 200us after IR LED goes on. */
    LL_TIM_OC_EnablePreload(TIM3, LL_TIM_CHANNEL_CH4);
    LL_TIM_CC_EnableChannel(TIM3, LL_TIM_CHANNEL_CH4);
    LL_TIM_SetTriggerOutput(TIM3, LL_TIM_TRGO_OC4REF); /* Trigger output on OC4 for ADC. */

    LL_TIM_EnableAllOutputs(TIM3);

    // Configure TIM3 as slave triggered by TIM1
    LL_TIM_SetTriggerInput(TIM3, LL_TIM_TS_ITR0); // ITR0 = TIM1
    LL_TIM_SetSlaveMode(TIM3, LL_TIM_SLAVEMODE_TRIGGER);

    // Enable interrupt for TIM3 Update
    LL_TIM_EnableIT_UPDATE(TIM3);
    NVIC_SetPriority(TIM3_IRQn, 2);
    NVIC_EnableIRQ(TIM3_IRQn);

    // Timer started by master.
    // LL_TIM_EnableCounter(TIM3);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Initializes ADC1 for reading the IR sensors.
 *
 */
static void of_hal_adc1_init(void)
{
    // Enable ADC1 & DMA1 clock
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_ADC1);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode                = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Speed               = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pull                = LL_GPIO_PULL_NO;

    // Configure PA5 through PA7 as analog inputs for IR sensors.
    GPIO_InitStruct.Pin = LL_GPIO_PIN_5 | LL_GPIO_PIN_6 | LL_GPIO_PIN_7; // ADC1_IN5, ADC1_IN6, ADC1_IN7
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // ADC calibration (equivalent to HAL_ADCEx_Calibration_Start)
    if (LL_ADC_IsEnabled(ADC1) == 0) {
        LL_ADC_StartCalibration(ADC1);
        while (LL_ADC_IsCalibrationOnGoing(ADC1) != 0)
            ; // Wait for calibration to complete
        // Delay after calibration (>= 4 ADC clocks)
        LL_mDelay(1);
    }

    // Configure ADC1
    LL_ADC_SetClock(ADC1, LL_ADC_CLOCK_SYNC_PCLK_DIV1);
    LL_ADC_SetResolution(ADC1, LL_ADC_RESOLUTION_10B);
    LL_ADC_SetDataAlignment(ADC1, LL_ADC_DATA_ALIGN_RIGHT);
    LL_ADC_REG_SetSequencerScanDirection(ADC1, LL_ADC_REG_SEQ_SCAN_DIR_FORWARD);
    LL_ADC_SetLowPowerMode(ADC1, LL_ADC_LP_MODE_NONE);
    LL_ADC_REG_SetContinuousMode(ADC1, LL_ADC_REG_CONV_SINGLE);
    LL_ADC_REG_SetSequencerDiscont(ADC1, LL_ADC_REG_SEQ_DISCONT_DISABLE);
    LL_ADC_REG_SetTriggerSource(ADC1, LL_ADC_REG_TRIG_EXT_TIM3_TRGO); // Trigger from TIM3 TRGO (OC4)
    LL_ADC_REG_SetTriggerEdge(ADC1, LL_ADC_REG_TRIG_EXT_RISING);
    LL_ADC_REG_SetDMATransfer(ADC1, LL_ADC_REG_DMA_TRANSFER_UNLIMITED);
    LL_ADC_REG_SetOverrun(ADC1, LL_ADC_REG_OVR_DATA_OVERWRITTEN);
    LL_ADC_SetSamplingTimeCommonChannels(ADC1, LL_ADC_SAMPLINGTIME_41CYCLES_5);

    // configuring channels 5 to 7
    LL_ADC_REG_SetSequencerChannels(ADC1, LL_ADC_CHANNEL_5 | LL_ADC_CHANNEL_6 | LL_ADC_CHANNEL_7);

    // Enable ADC
    LL_ADC_Enable(ADC1);

    // LL_ADC_EnableIT_EOC(ADC1);
    // NVIC_SetPriority(ADC_COMP_IRQn, 2);
    // NVIC_EnableIRQ(ADC_COMP_IRQn);

    // Remap ADC1 to DMA1 Channel 1
    LL_SYSCFG_SetDMARemap_CH1(LL_SYSCFG_DMA_MAP_ADC);

    // Configure DMA1 Channel1 for ADC1
    LL_DMA_InitTypeDef DMA_InitStruct     = {0};
    DMA_InitStruct.PeriphOrM2MSrcAddress  = (uint32_t)&ADC1->DR;
    DMA_InitStruct.MemoryOrM2MDstAddress  = (uint32_t)adc_dma_buf;
    DMA_InitStruct.Direction              = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
    DMA_InitStruct.Mode                   = LL_DMA_MODE_CIRCULAR;
    DMA_InitStruct.PeriphOrM2MSrcIncMode  = LL_DMA_PERIPH_NOINCREMENT;
    DMA_InitStruct.MemoryOrM2MDstIncMode  = LL_DMA_MEMORY_INCREMENT;
    DMA_InitStruct.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_HALFWORD;
    DMA_InitStruct.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_WORD;
    DMA_InitStruct.NbData                 = ENCODER_CHANNEL_COUNT;
    DMA_InitStruct.Priority               = LL_DMA_PRIORITY_HIGH;

    LL_DMA_Init(DMA1, LL_DMA_CHANNEL_1, &DMA_InitStruct);

    // Configure and enable NVIC for DMA interrupt
    // LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_1);
    // NVIC_SetPriority(DMA1_Channel1_IRQn, 2);
    // NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    // Enable DMA channel
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);

    // Start the ADC conversions.
    LL_ADC_REG_StartConversion(ADC1);
}

//----------------------------------------------------------------------------------------------------------------------

static void of_hal_uart1_init(void)
{
    // Enable UART1 clock
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_USART1);

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode                = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed               = LL_GPIO_SPEED_FREQ_HIGH;

    // Configure PA3 (UART1_RX_TOP).
    GPIO_InitStruct.Pin       = LL_GPIO_PIN_3; // UART1_RX
    GPIO_InitStruct.Alternate = LL_GPIO_AF_1;  // UART1
    GPIO_InitStruct.Pull      = LL_GPIO_PULL_UP;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Configure PB6 (UART1_TX_BOT).
    GPIO_InitStruct.Pin       = LL_GPIO_PIN_6; // UART1_TX
    GPIO_InitStruct.Alternate = LL_GPIO_AF_0;  // UART1
    GPIO_InitStruct.Pull      = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // Configure PA2 (UART1_TX_TOP). This pin can be used as a secondary TX when the module is at a column end.
    GPIO_InitStruct.Pin  = LL_GPIO_PIN_2; // alternate UART1_TX,
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    // Set PA2 to AF1 (UART1_TX) (Can't be done through struct because it's input)
    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_2, LL_GPIO_AF_1);

    // Configure UART1
    LL_USART_InitTypeDef UART_InitStruct = {0};
    UART_InitStruct.BaudRate             = 115200;
    UART_InitStruct.DataWidth            = LL_USART_DATAWIDTH_8B;
    UART_InitStruct.StopBits             = LL_USART_STOPBITS_1;
    UART_InitStruct.Parity               = LL_USART_PARITY_NONE;
    UART_InitStruct.TransferDirection    = LL_USART_DIRECTION_TX_RX;
    UART_InitStruct.HardwareFlowControl  = LL_USART_HWCONTROL_NONE;
    UART_InitStruct.OverSampling         = LL_USART_OVERSAMPLING_16;

    LL_USART_Init(USART1, &UART_InitStruct);

    LL_USART_Enable(USART1);

    // Configure DMA1 Channel2 for USART1_TX
    LL_DMA_InitTypeDef DMA_TX_InitStruct     = {0};
    DMA_TX_InitStruct.PeriphOrM2MSrcAddress  = (uint32_t)&USART1->DR;
    DMA_TX_InitStruct.MemoryOrM2MDstAddress  = (uint32_t)uart_tx_dma_buf;
    DMA_TX_InitStruct.Direction              = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
    DMA_TX_InitStruct.Mode                   = LL_DMA_MODE_NORMAL;
    DMA_TX_InitStruct.PeriphOrM2MSrcIncMode  = LL_DMA_PERIPH_NOINCREMENT;
    DMA_TX_InitStruct.MemoryOrM2MDstIncMode  = LL_DMA_MEMORY_INCREMENT;
    DMA_TX_InitStruct.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
    DMA_TX_InitStruct.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;
    DMA_TX_InitStruct.NbData                 = UART_DMA_BUF_LEN;
    DMA_TX_InitStruct.Priority               = LL_DMA_PRIORITY_HIGH;
    LL_DMA_Init(DMA1, LL_DMA_CHANNEL_2, &DMA_TX_InitStruct);

    // Remap USART1_TX to DMA1 Channel 2
    LL_SYSCFG_SetDMARemap_CH2(LL_SYSCFG_DMA_MAP_USART1_TX);

    // Configure DMA1 Channel3 for USART1_RX
    LL_DMA_InitTypeDef DMA_RX_InitStruct     = {0};
    DMA_RX_InitStruct.PeriphOrM2MSrcAddress  = (uint32_t)&USART1->DR;
    DMA_RX_InitStruct.MemoryOrM2MDstAddress  = (uint32_t)uart_rx_dma_buf;
    DMA_RX_InitStruct.Direction              = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
    DMA_RX_InitStruct.Mode                   = LL_DMA_MODE_CIRCULAR;
    DMA_RX_InitStruct.PeriphOrM2MSrcIncMode  = LL_DMA_PERIPH_NOINCREMENT;
    DMA_RX_InitStruct.MemoryOrM2MDstIncMode  = LL_DMA_MEMORY_INCREMENT;
    DMA_RX_InitStruct.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
    DMA_RX_InitStruct.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;
    DMA_RX_InitStruct.NbData                 = UART_DMA_BUF_LEN;
    DMA_RX_InitStruct.Priority               = LL_DMA_PRIORITY_HIGH;
    LL_DMA_Init(DMA1, LL_DMA_CHANNEL_3, &DMA_RX_InitStruct);

    // Enable USART1 DMA requests
    LL_USART_EnableDMAReq_RX(USART1);
    LL_USART_EnableDMAReq_TX(USART1);

    // Remap USART1_RX to DMA1 Channel 3
    LL_SYSCFG_SetDMARemap_CH3(LL_SYSCFG_DMA_MAP_USART1_RX);

    // Enable DMA interrupts if needed
    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_2);
    NVIC_SetPriority(DMA1_Channel2_3_IRQn, 3);
    NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_3); // Enable DMA channel for USART1_RX

    // LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_3);
    // NVIC_SetPriority(DMA1_Channel3_IRQn, 2);
    // NVIC_EnableIRQ(DMA1_Channel3_IRQn);
}

//-----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Gets the write pointer for the UART DMA buffer.
 *
 * This function calculates the number of bytes written to the UART TX DMA buffer
 * and returns the write pointer, which is the current position in the buffer.
 *
 * @return Pointer to the current write position in the UART TX DMA buffer.
 */
static void *of_hal_uart_dma_w_ptr_get(void)
{
    uint32_t offset = UART_DMA_BUF_LEN - LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_3);
    return (void *)(uart_rx_dma_buf + offset);
}

//-----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Starts a UART tx DMA transfer.
 *
 * @param data Pointer to the data to be transmitted.
 * @param length Length of the data to be transmitted.
 */

static void of_hal_uart_tx_dma_start(size_t length)
{
    // Disable DMA channel first
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_2);

    // Set the memory data length, address is already set in of_hal_uart1_init
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, length);

    // Enable DMA channel to start transfer
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_2);
}
