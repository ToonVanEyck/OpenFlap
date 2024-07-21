#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "py32f0xx_hal.h"
#include "rbuff.h"

typedef struct uart_driver_ctx_tag {
    UART_HandleTypeDef *huart;
    rbuff_t rx_rbuff;
    rbuff_t tx_rbuff;
    uint8_t rx_tmp_buff;
    uint8_t tx_tmp_buff;
} uart_driver_ctx_t;

/**
 * \brief Initialize the UART driver.
 *
 * \param[inout] uart_driver The UART driver.
 * \param[in] rx_buff The raw array to be used by the driver RX ringbuffer.
 * \param[in] rx_buff_size The size of the raw array \p rx_buff.
 * \param[in] tx_buff The raw array to be used by the driver TX ringbuffer.
 * \param[in] tx_buff_size The size of the raw array \p tx_buff.
 */
void uart_driver_init(uart_driver_ctx_t *uart_driver, UART_HandleTypeDef *huart, uint8_t *rx_buff, uint8_t rx_buff_size,
                      uint8_t *tx_buff, uint8_t tx_buff_size);

/**
 * \brief The RX interrupt service routine to be used for the UART driver RX interrupt.
 *
 * \param[inout] uart_driver The UART driver.
 */
void uart_driver_rx_isr(uart_driver_ctx_t *uart_driver);

/**
 * \brief The TX interrupt service routine to be used for the UART driver TX interrupt.
 *
 * \param[inout] uart_driver The UART driver.
 */
void uart_driver_ctx_tx_isr(uart_driver_ctx_t *uart_driver);

/**
 * \brief Read data from the UART driver.
 *
 * \param[inout] uart_driver The UART driver.
 * \param[out] data The data read from the driver.
 * \param[in] size The number of bytes to read.
 *
 * \return The number of bytes read.
 */
uint8_t uart_driver_read(uart_driver_ctx_t *uart_driver, uint8_t *data, uint8_t size);

/**
 * \brief Count the number of bytes that can be read from the UART driver.
 *
 * \param[inout] uart_driver The UART driver.
 * \return The number of bytes that can be read from the driver.
 */
uint8_t uart_driver_cnt_readable(uart_driver_ctx_t *uart_driver);

/**
 * \brief Write data to the UART driver.
 *
 * \param[inout] uart_driver The UART driver.
 * \param[in] data The data to be written to the driver.
 * \param[in] size The number of bytes to write.
 *
 * \return The number of bytes written.
 */
uint8_t uart_driver_write(uart_driver_ctx_t *uart_driver, uint8_t *data, uint8_t size);

/**
 * \brief Count the number of bytes that can be written to the UART driver.
 *
 * \param[inout] uart_driver The UART driver.
 * \return The number of bytes that can be written to the driver.
 */
uint8_t uart_driver_cnt_writable(uart_driver_ctx_t *uart_driver);

/**
 * \brief Count the number of bytes that have been written to the UART driver.
 *
 * \param[inout] uart_driver The UART driver.
 * \return The number of bytes that have been written to the driver.
 */
uint8_t uart_driver_cnt_written(uart_driver_ctx_t *uart_driver);

/**
 * \brief Check if the UART driver is busy.
 * The driver is considered busy if the RX and TX ringbuffers are not empty.
 *
 * \param[inout] uart_driver The UART driver.
 * \return True if the driver is busy, otherwise false.
 */
bool uart_driver_is_busy(uart_driver_ctx_t *uart_driver);