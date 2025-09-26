/**
 * @file openflap_hal_cct.c
 * This version of the hardware abstraction is used for the Chain Communication Test board.
 */

#include "openflap_hal.h"
#include "flash.h"
#include "rbuff.h"
#include "uart_driver.h"

//======================================================================================================================
//                                                  DEFINES AND CONSTS
//======================================================================================================================

/** Number of debug GPIO pins. */
#define DEBUG_GPIO_PINS_COUNT (4)
/** Debug GPIO pins. */
const uint32_t DEBUG_GPIO_PINS[DEBUG_GPIO_PINS_COUNT] = {LL_GPIO_PIN_4, LL_GPIO_PIN_5, LL_GPIO_PIN_6, LL_GPIO_PIN_7};
/** Debug GPIO ports. */
GPIO_TypeDef *const DEBUG_GPIO_PORTS[DEBUG_GPIO_PINS_COUNT] = {GPIOA, GPIOA, GPIOA, GPIOA};

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

void of_hal_motor_control(int16_t speed, int16_t decay)
{
}

//----------------------------------------------------------------------------------------------------------------------

bool of_hal_motor_is_running(void)
{
    return false;
}

//----------------------------------------------------------------------------------------------------------------------

bool of_hal_is_column_end(void)
{
    return LL_GPIO_IsInputPinSet(GPIOA, LL_GPIO_PIN_12);
}

//----------------------------------------------------------------------------------------------------------------------

void of_hal_encoder_values_get(uint16_t *encoder_values)
{
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

void of_hal_ir_timer_idle_set(bool idle)
{
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

    /* Initialize the 4 debug pins. */
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

    // Configure TIM1 base: 24MHz / (1000 * 24) ≈ 1000Hz (1000 counts of 1us)
    LL_TIM_SetPrescaler(TIM1, 24 - 1);
    LL_TIM_SetAutoReload(TIM1, 1000 - 1);
    LL_TIM_SetCounterMode(TIM1, LL_TIM_COUNTERMODE_UP);

    LL_TIM_EnableAllOutputs(TIM1);

    // Configure TIM1 as master - trigger output on update event
    LL_TIM_SetTriggerOutput(TIM1, LL_TIM_TRGO_UPDATE);

    // Enable interrupt for TIM1 Update
    LL_TIM_EnableIT_UPDATE(TIM1);
    NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, 2);
    NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);
    NVIC_SetPriority(TIM1_CC_IRQn, 2);
    NVIC_EnableIRQ(TIM1_CC_IRQn);

    // Enable the timer (master timer starts first)
    LL_TIM_EnableCounter(TIM1);
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

    // Configure PA2 (UART1_TX) and PA3 (UART1_RX) as alternate function for UART1
    GPIO_InitStruct.Pin       = LL_GPIO_PIN_2 | LL_GPIO_PIN_3; // UART1_TX, UART1_RX
    GPIO_InitStruct.Alternate = LL_GPIO_AF_1;                  // UART1
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

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
