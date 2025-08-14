#include "openflap_hal.h"
#include "flash.h"
#include "rbuff.h"
#include "uart_driver.h"

//======================================================================================================================
//                                                  DEFINES AND CONSTS
//======================================================================================================================

/** Number of debug GPIO pins. */
#define DEBUG_GPIO_PINS_COUNT (1)
/** Debug GPIO pins. */
const uint32_t DEBUG_GPIO_PINS[DEBUG_GPIO_PINS_COUNT] = {LL_GPIO_PIN_0};
/** Debug GPIO ports. */
GPIO_TypeDef *const DEBUG_GPIO_PORTS[DEBUG_GPIO_PINS_COUNT] = {GPIOF};

/** Flash memory offset for config. */
extern uint32_t __FLASH_NVS_START__;
#define NVS_START_ADDR ((uint32_t)&__FLASH_NVS_START__)

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
    if (pin >= DEBUG_GPIO_PINS_COUNT) {
        return; // Invalid pin number
    }
    if (value) {
        LL_GPIO_SetOutputPin(DEBUG_GPIO_PORTS[pin], DEBUG_GPIO_PINS[pin]);
    } else {
        LL_GPIO_ResetOutputPin(DEBUG_GPIO_PORTS[pin], DEBUG_GPIO_PINS[pin]);
    }
}

//----------------------------------------------------------------------------------------------------------------------

void of_hal_debug_pin_toggle(uint8_t pin)
{
    if (pin >= DEBUG_GPIO_PINS_COUNT) {
        return; // Invalid pin number
    }
    LL_GPIO_TogglePin(DEBUG_GPIO_PORTS[pin], DEBUG_GPIO_PINS[pin]);
}

//----------------------------------------------------------------------------------------------------------------------

bool of_hal_debug_pin_get(uint8_t pin)
{
    if (pin >= DEBUG_GPIO_PINS_COUNT) {
        return false; // Invalid pin number
    }
    return LL_GPIO_IsOutputPinSet(DEBUG_GPIO_PORTS[pin], DEBUG_GPIO_PINS[pin]);
}

//----------------------------------------------------------------------------------------------------------------------

void of_hal_motor_control(of_hal_motor_ctx_t *motor)
{
    if (motor->speed > 1000) {
        motor->speed = 1000; // Clamp speed to maximum value
    } else if (motor->speed < 0) {
        motor->speed = 0; // Clamp speed to minimum value
    }

    switch (motor->mode) {
        case MOTOR_FORWARD:
            if (motor->decay_mode == MOTOR_DECAY_FAST) {
                LL_TIM_OC_SetCompareCH1(TIM3, 0);
                LL_TIM_OC_SetCompareCH2(TIM3, motor->speed * 1);
            } else {
                LL_TIM_OC_SetCompareCH1(TIM3, 1000 - (motor->speed * 1));
                LL_TIM_OC_SetCompareCH2(TIM3, 1000);
            }
            break;
        case MOTOR_REVERSE:
            if (motor->decay_mode == MOTOR_DECAY_FAST) {
                LL_TIM_OC_SetCompareCH1(TIM3, motor->speed * 1);
                LL_TIM_OC_SetCompareCH2(TIM3, 0);
            } else {
                LL_TIM_OC_SetCompareCH1(TIM3, 1000);
                LL_TIM_OC_SetCompareCH2(TIM3, 1000 - (motor->speed * 1));
            }
            break;
        case MOTOR_IDLE:
            LL_TIM_OC_SetCompareCH1(TIM3, 0);
            LL_TIM_OC_SetCompareCH2(TIM3, 0);
            break;
        case MOTOR_BRAKE:
            LL_TIM_OC_SetCompareCH1(TIM3, 1000);
            LL_TIM_OC_SetCompareCH2(TIM3, 1000);
            break;
    }
}

//----------------------------------------------------------------------------------------------------------------------

bool of_hal_is_column_end(void)
{
    return LL_GPIO_IsInputPinSet(GPIOA, LL_GPIO_PIN_1);
}

//----------------------------------------------------------------------------------------------------------------------

void of_hal_encoder_values_get(uint16_t *encoder_values)
{
    // Map the dma buffer to the encoder channels.
    encoder_values[1] = adc_dma_buf[0]; // Encoder channel A
    encoder_values[0] = adc_dma_buf[1]; // Encoder channel B
    encoder_values[2] = adc_dma_buf[2]; // Encoder channel C
    encoder_values[3] = adc_dma_buf[3]; // Encoder channel D
    encoder_values[4] = adc_dma_buf[4]; // Encoder channel Z
}

//----------------------------------------------------------------------------------------------------------------------

void of_hal_uart_tx_pin_update(bool enable_secondary_tx)
{
    static bool secondary_tx_enabled = false;

    if (enable_secondary_tx != secondary_tx_enabled) {
        secondary_tx_enabled = enable_secondary_tx;
        LL_GPIO_SetPinMode(GPIOF, LL_GPIO_PIN_1, enable_secondary_tx ? LL_GPIO_MODE_ALTERNATE : LL_GPIO_MODE_INPUT);
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

    /* Initialize the "power-ok" detection pin. (PB5) */
    // GPIO_InitStruct.Pin   = LL_GPIO_PIN_5;
    // GPIO_InitStruct.Mode  = LL_GPIO_MODE_INPUT;
    // GPIO_InitStruct.Pull  = LL_GPIO_NO_PULL;
    // GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    // LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Initialize the 1 debug pins. */
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_HIGH;
    for (int i = 0; i < DEBUG_GPIO_PINS_COUNT; i++) {
        GPIO_InitStruct.Pin = DEBUG_GPIO_PINS[i];
        LL_GPIO_Init(DEBUG_GPIO_PORTS[i], &GPIO_InitStruct);
    }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Initializes TIM1 for IR LED control.
 * TIMER 1 is configure as master at 1kHz.
 */

static void of_hal_tim1_init(void)
{
    // Enable TIM1 Clock
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_TIM1);

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode                = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.OutputType          = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull                = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Speed               = LL_GPIO_SPEED_FREQ_HIGH;

    // TODO: PWM pin is disabled due to hardware changes, using interrupt for now.
    // Configure PB5 as output.
    GPIO_InitStruct.Pin = LL_GPIO_PIN_5;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // Configure TIM1 base: 24MHz / (1000 * 24) ≈ 1000Hz (1000 counts of 1us)
    LL_TIM_SetPrescaler(TIM1, 24 - 1);
    LL_TIM_SetAutoReload(TIM1, 1000 - 1);
    LL_TIM_SetCounterMode(TIM1, LL_TIM_COUNTERMODE_UP);

    // Configure PWM mode for Channel 3 (IR LED)
    LL_TIM_OC_SetMode(TIM1, LL_TIM_CHANNEL_CH3, LL_TIM_OCMODE_PWM1);
    LL_TIM_OC_SetPolarity(TIM1, LL_TIM_CHANNEL_CH3, LL_TIM_OCPOLARITY_LOW);
    LL_TIM_OC_SetCompareCH3(TIM1, 1000 - 220); /* IR LED should be on for 220us each cycle. */
    LL_TIM_OC_EnablePreload(TIM1, LL_TIM_CHANNEL_CH3);
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH3);

    // Configure Channel 4 as output compare for external trigger (ADC trigger) Not connected to physical pin.
    LL_TIM_OC_SetMode(TIM1, LL_TIM_CHANNEL_CH4, LL_TIM_OCMODE_PWM2); /* Generates a rising edge after 200us. */
    LL_TIM_OC_SetPolarity(TIM1, LL_TIM_CHANNEL_CH4, LL_TIM_OCPOLARITY_LOW);
    LL_TIM_OC_SetCompareCH4(TIM1, 1000 - 20); /* ADC should start 200us after IR LED goes on. */
    LL_TIM_OC_EnablePreload(TIM1, LL_TIM_CHANNEL_CH4);
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH4);
    LL_TIM_SetTriggerOutput(TIM1, LL_TIM_TRGO_OC4REF);

    LL_TIM_EnableAllOutputs(TIM1);

    // Configure TIM1 as master - trigger output on update event
    LL_TIM_SetTriggerOutput(TIM1, LL_TIM_TRGO_UPDATE);

    // Enable interrupt for TIM1 Update
    LL_TIM_EnableIT_UPDATE(TIM1);
    LL_TIM_EnableIT_CC3(TIM1);
    NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, 2);
    NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);
    NVIC_SetPriority(TIM1_CC_IRQn, 2);
    NVIC_EnableIRQ(TIM1_CC_IRQn);

    // Enable the timer (master timer starts first)
    LL_TIM_EnableCounter(TIM1);
}

//-----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Initializes TIM3 for PWM on channel 1 and 2.
 * TIM3 is configured as a slave at 200Hz.
 */
static void of_hal_tim3_init(void)
{

    // Enable TIM3 Clock
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode                = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed               = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pull                = LL_GPIO_PULL_DOWN;

    // Configure PA6 (TIM3_CH1) and PA7 (TIM3_CH2) as alternate function for PWM.
    GPIO_InitStruct.Pin       = LL_GPIO_PIN_6 | LL_GPIO_PIN_7;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_1; // TIM3_CH1 & TIM3_CH2
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Configure TIM3 base: 24MHz / (1000 * 120) ≈ 200Hz (1000 counts of 5us)
    LL_TIM_SetPrescaler(TIM3, 120 - 1);
    LL_TIM_SetAutoReload(TIM3, 1000 - 1);
    LL_TIM_SetCounterMode(TIM3, LL_TIM_COUNTERMODE_UP);

    // Configure TIM3 as slave to TIM1 Set TIM3 to be triggered by TIM1 TRGO(Internal Trigger 0 = ITR0 = TIM1)
    LL_TIM_SetTriggerInput(TIM3, LL_TIM_TS_ITR0);
    LL_TIM_SetSlaveMode(TIM3, LL_TIM_SLAVEMODE_TRIGGER);

    // Configure PWM mode for Channel 1 and 2 (motor)
    LL_TIM_OC_SetMode(TIM3, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_PWM1);
    LL_TIM_OC_SetPolarity(TIM3, LL_TIM_CHANNEL_CH1, LL_TIM_OCPOLARITY_HIGH);
    LL_TIM_OC_SetCompareCH1(TIM3, 0);
    LL_TIM_OC_EnablePreload(TIM3, LL_TIM_CHANNEL_CH1);
    LL_TIM_CC_EnableChannel(TIM3, LL_TIM_CHANNEL_CH1);

    LL_TIM_OC_SetMode(TIM3, LL_TIM_CHANNEL_CH2, LL_TIM_OCMODE_PWM1);
    LL_TIM_OC_SetPolarity(TIM3, LL_TIM_CHANNEL_CH2, LL_TIM_OCPOLARITY_HIGH);
    LL_TIM_OC_SetCompareCH2(TIM3, 0);
    LL_TIM_OC_EnablePreload(TIM3, LL_TIM_CHANNEL_CH2);
    LL_TIM_CC_EnableChannel(TIM3, LL_TIM_CHANNEL_CH2);

    // Enable interrupt for TIM3 Update
    LL_TIM_EnableIT_UPDATE(TIM3);
    NVIC_SetPriority(TIM3_IRQn, 2);
    NVIC_EnableIRQ(TIM3_IRQn);

    // Timer started by TIM1 trigger
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

    // Configure PA0 through PA4 as analog inputs for IR sensors.
    GPIO_InitStruct.Pin = LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 | LL_GPIO_PIN_3 |
                          LL_GPIO_PIN_4; // ADC1_IN0, ADC1_IN1, ADC1_IN2, ADC1_IN3, ADC1_IN4
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
    LL_ADC_REG_SetTriggerSource(ADC1, LL_ADC_REG_TRIG_EXT_TIM1_TRGO); // TODO: change to TIM3_TRGO for new hardware
    LL_ADC_REG_SetTriggerEdge(ADC1, LL_ADC_REG_TRIG_EXT_RISING);
    LL_ADC_REG_SetDMATransfer(ADC1, LL_ADC_REG_DMA_TRANSFER_UNLIMITED);
    LL_ADC_REG_SetOverrun(ADC1, LL_ADC_REG_OVR_DATA_OVERWRITTEN);
    LL_ADC_SetSamplingTimeCommonChannels(ADC1, LL_ADC_SAMPLINGTIME_41CYCLES_5);

    // configuring channels 0 to 4
    LL_ADC_REG_SetSequencerChannels(ADC1, LL_ADC_CHANNEL_0 | LL_ADC_CHANNEL_1 | LL_ADC_CHANNEL_2 | LL_ADC_CHANNEL_3 |
                                              LL_ADC_CHANNEL_4);

    // Enable ADC
    LL_ADC_Enable(ADC1);

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

    // Enable DMA channel
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);

    // Configure and enable NVIC for DMA interrupt
    // LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_1);
    // NVIC_SetPriority(DMA1_Channel1_IRQn, 2);
    // NVIC_EnableIRQ(DMA1_Channel1_IRQn);

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
    GPIO_InitStruct.Pull                = LL_GPIO_PULL_NO;

    // Configure PB6 (UART1_TX) and PB7 (UART1_RX) as alternate function for UART1
    GPIO_InitStruct.Pin       = LL_GPIO_PIN_6 | LL_GPIO_PIN_7; // UART1_TX, UART1_RX
    GPIO_InitStruct.Alternate = LL_GPIO_AF_0;                  // UART1
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // Configure PF1 (UART1_TX) as input. This pin can be used as a secondary TX when the module is at a column end.
    GPIO_InitStruct.Pin  = LL_GPIO_PIN_1; // alternate UART1_TX,
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    LL_GPIO_Init(GPIOF, &GPIO_InitStruct);
    // Set PF1 to AF8 (UART1_TX) (Can't be done through struct because it's input)
    LL_GPIO_SetAFPin_0_7(GPIOF, LL_GPIO_PIN_1, LL_GPIO_AF_8);

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
    NVIC_SetPriority(DMA1_Channel2_3_IRQn, 2);
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
