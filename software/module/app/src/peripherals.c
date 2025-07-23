#include "peripherals.h"
#include "rbuff.h"
#include "uart_driver.h"

//======================================================================================================================
//                                                  DEFINES AND CONSTS
//======================================================================================================================

/** Number of debug GPIO pins. */
#define DEBUG_GPIO_PINS_COUNT (2)
/** Debug GPIO pins. */
const uint32_t DEBUG_GPIO_PINS[DEBUG_GPIO_PINS_COUNT] = {LL_GPIO_PIN_0, LL_GPIO_PIN_1};
/** Debug GPIO ports. */
GPIO_TypeDef *const DEBUG_GPIO_PORTS[DEBUG_GPIO_PINS_COUNT] = {GPIOF, GPIOF};

//======================================================================================================================
//                                                   GLOBAL VARIABLES
//======================================================================================================================

uint32_t systick_1ms_cnt    = 0; /**< Counter for SysTick events. */
uint32_t pwm_timer_tick_cnt = 0; /**< Counter for TIM3 update events. */

/* ADC Input Capture DMA Buffer */
#define ADC_DMA_BUF_LEN 3
volatile uint32_t adc_dma_buf[ADC_DMA_BUF_LEN] = {0};

/* UART DMA buffers */
#define UART_DMA_BUF_LEN 64
volatile uint8_t uart_rx_dma_buf[UART_DMA_BUF_LEN] = {0}; /**< UART RX DMA ring buffer */
uint8_t uart_tx_buffer[UART_DMA_BUF_LEN]           = {0}; /**< UART TX buffer for ring buffer */
volatile uint8_t uart_tx_dma_buf[UART_DMA_BUF_LEN] = {0}; /**< UART TX DMA buffer */

//======================================================================================================================
//                                                  FUNCTION PROTOTYPES
//======================================================================================================================

static void peripheral_gpio_init(void);  /**< GPIO initialization. */
static void peripheral_tim3_init(void);  /**< TIM3 initialization. (Motor PWM) */
static void peripheral_adc1_init(void);  /**< ADC1 initialization. (IR sensors) */
static void peripheral_uart1_init(void); /**< UART1 initialization. (chain comm) */

static void *peripheral_uart_dma_w_ptr_get(void);        /**< Get the write pointer for UART DMA buffer. */
static void peripheral_uart_tx_dma_start(size_t length); /**< Start a DMA transfer for UART TX. */

//======================================================================================================================
//                                                   PUBLIC FUNCTIONS
//======================================================================================================================

void peripherals_init(peripherals_ctx_t *peripherals_ctx)
{
    BSP_RCC_HSI_24MConfig();

    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);

    LL_SYSTICK_EnableIT(); // Enable SysTick interrupt

    uart_driver_init(&peripherals_ctx->uart_driver, uart_rx_dma_buf, UART_DMA_BUF_LEN, uart_tx_buffer, UART_DMA_BUF_LEN,
                     uart_tx_dma_buf, UART_DMA_BUF_LEN, peripheral_uart_dma_w_ptr_get, peripheral_uart_tx_dma_start);

    peripheral_gpio_init();
    peripheral_tim3_init();
    peripheral_adc1_init();
    peripheral_uart1_init();
}

//----------------------------------------------------------------------------------------------------------------------

void peripherals_deinit(void)
{
}

//-----------------------------------------------------------------------------------------------------------------------

uint32_t get_tick_count(void)
{
    return systick_1ms_cnt;
}

//----------------------------------------------------------------------------------------------------------------------

uint32_t get_pwm_tick_count(void)
{
    return pwm_timer_tick_cnt;
}

//------------------------------------------------------------------------------------------------------------------------

void debug_pin_set(uint8_t pin, bool value)
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

void debug_pin_toggle(uint8_t pin)
{
    if (pin >= DEBUG_GPIO_PINS_COUNT) {
        return; // Invalid pin number
    }
    LL_GPIO_TogglePin(DEBUG_GPIO_PORTS[pin], DEBUG_GPIO_PINS[pin]);
}

//----------------------------------------------------------------------------------------------------------------------

bool debug_pin_get(uint8_t pin)
{
    if (pin >= DEBUG_GPIO_PINS_COUNT) {
        return false; // Invalid pin number
    }
    return LL_GPIO_IsOutputPinSet(DEBUG_GPIO_PORTS[pin], DEBUG_GPIO_PINS[pin]);
}

//----------------------------------------------------------------------------------------------------------------------

void motor_control(motor_ctx_t *motor)
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

bool is_column_end(void)
{
    return LL_GPIO_IsInputPinSet(GPIOA, LL_GPIO_PIN_1);
}

//----------------------------------------------------------------------------------------------------------------------

void encoder_states_get(encoder_states_t *states, uint16_t adc_lower_threshold, uint16_t adc_upper_threshold)
{
    // Read the ADC values for the encoder channels
    states->enc_raw_z = adc_dma_buf[0];
    states->enc_raw_b = adc_dma_buf[1];
    states->enc_raw_a = adc_dma_buf[2];

    // Determine the states based on the ADC values and threshold
    states->enc_a =
        (states->enc_a) ? (states->enc_raw_a < adc_upper_threshold) : (states->enc_raw_a < adc_lower_threshold);
    states->enc_b =
        (states->enc_b) ? (states->enc_raw_b < adc_upper_threshold) : (states->enc_raw_b < adc_lower_threshold);
    states->enc_z =
        (states->enc_z) ? (states->enc_raw_z < adc_upper_threshold) : (states->enc_raw_z < adc_lower_threshold);
}

//======================================================================================================================
//                                                         PRIVATE FUNCTIONS
//======================================================================================================================

/**
 * @brief Initializes GPIOs used for debugging.
 *
 * This function configures 2 GPIO pins as outputs for debugging purposes.
 * The pins are set to high speed and pull-up mode.
 */
static void peripheral_gpio_init(void)
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable GPIO clocks. */
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOF);

    /* Initialize the column end detection pin. (PA1) */
    GPIO_InitStruct.Pin   = LL_GPIO_PIN_1;
    GPIO_InitStruct.Mode  = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Initialize the IR sense enable pin. (PA2) */
    GPIO_InitStruct.Pin   = LL_GPIO_PIN_2;
    GPIO_InitStruct.Mode  = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pull  = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Initialize the 2 debug pins. */
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
 * @brief Initializes TIM3 for PWM on channel 1 and 2.
 */
static void peripheral_tim3_init(void)
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
    // Configure PA4 (TIM3_CH3) as alternate function for PWM.
    GPIO_InitStruct.Pin       = LL_GPIO_PIN_4;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_13; // TIM3_CH3
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Configure TIM3 base: 24MHz / (1000 * 24) ≈ 1000Hz (0000 counts of 1µS)
    LL_TIM_SetPrescaler(TIM3, 24 - 1);
    LL_TIM_SetAutoReload(TIM3, 1000 - 1);
    LL_TIM_SetCounterMode(TIM3, LL_TIM_COUNTERMODE_UP);

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

    // Configure PWM mode for Channel 3 (IR led)
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
    LL_TIM_SetTriggerOutput(TIM3, LL_TIM_TRGO_OC4REF);

    // Enable interrupt for TIM3 Update
    LL_TIM_EnableIT_UPDATE(TIM3);
    NVIC_SetPriority(TIM3_IRQn, 2);
    NVIC_EnableIRQ(TIM3_IRQn);

    LL_TIM_EnableCounter(TIM3);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Initializes ADC1 for reading the IR sensors.
 *
 */
static void peripheral_adc1_init(void)
{
    // Enable ADC1 & DMA1 clock
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_ADC1);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode                = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Speed               = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pull                = LL_GPIO_PULL_NO;

    // Configure PA2 (ADC1_IN2), PA3 (ADC1_IN3), and PA5 (ADC1_IN5) as analog inputs for IR sensors.
    GPIO_InitStruct.Pin = LL_GPIO_PIN_2 | LL_GPIO_PIN_3 | LL_GPIO_PIN_5; // ADC1_IN2, ADC1_IN3, ADC1_IN5
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
    LL_ADC_REG_SetTriggerSource(ADC1, LL_ADC_REG_TRIG_EXT_TIM3_TRGO);
    LL_ADC_REG_SetTriggerEdge(ADC1, LL_ADC_REG_TRIG_EXT_RISING);
    LL_ADC_REG_SetDMATransfer(ADC1, LL_ADC_REG_DMA_TRANSFER_UNLIMITED);
    LL_ADC_REG_SetOverrun(ADC1, LL_ADC_REG_OVR_DATA_OVERWRITTEN);
    LL_ADC_SetSamplingTimeCommonChannels(ADC1, LL_ADC_SAMPLINGTIME_41CYCLES_5);

    // configuring channels 2, 3 and 5
    LL_ADC_REG_SetSequencerChannels(ADC1, LL_ADC_CHANNEL_2 | LL_ADC_CHANNEL_3 | LL_ADC_CHANNEL_5);

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
    DMA_InitStruct.NbData                 = ADC_DMA_BUF_LEN;
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

static void peripheral_uart1_init(void)
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

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Gets the write pointer for the TIM1 input capture DMA buffer.
 *
 * This function calculates the number of bytes written to the DMA buffer
 * and returns the write pointer, which is the current position in the buffer.
 *
 * @return Pointer to the current write position in the TIM1 input capture DMA buffer.
 */
void *peripheral_tim1_dma_w_ptr_get(void)
{
    // uint32_t remaining = LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_1);
    // uint32_t written   = TIM1_IC_DMA_BUF_LEN - remaining;
    // return (void *)(tim1_ic_dma_buf + written);
    return NULL;
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
static void *peripheral_uart_dma_w_ptr_get(void)
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

static void peripheral_uart_tx_dma_start(size_t length)
{
    // Disable DMA channel first
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_2);

    // Set the memory data length, address is already set in peripheral_uart1_init
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, length);

    // Enable DMA channel to start transfer
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_2);
}
