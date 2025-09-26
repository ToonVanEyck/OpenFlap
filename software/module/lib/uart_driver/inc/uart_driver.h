#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "rbuff.h"

typedef void (*uart_tx_dma_start_cb)(size_t length);

typedef struct uart_driver_ctx_tag {
    rbuff_t rx_rbuff;
    rbuff_t tx_rbuff;
    bool tx_dma_busy;                     /** Flag indicating if the TX DMA is busy. */
    volatile uint8_t *tx_dma_buffer;      /**< Pointer to the TX DMA buffer used for writing data. */
    size_t tx_dma_buffer_size;            /**< Size of the TX DMA buffer. */
    uart_tx_dma_start_cb tx_dma_start_cb; /**< Callback to start the DMA transfer for TX. */
} uart_driver_ctx_t;

/**
 * \brief Initialize the UART driver.
 *
 * \param[inout] uart_driver The UART driver context to be initialized.
 * \param[in] rx_rbuff The RX ringbuffer to be used by the driver.
 * \param[in] rx_buff_size The size of the RX buffer.
 * \param[in] tx_rbuff The TX ringbuffer to be used by the driver.
 * \param[in] tx_buff_size The size of the TX buffer.
 * \param[in] tx_dma_buffer Pointer to the TX DMA buffer.
 * \param[in] tx_dma_buffer_size The size of the TX DMA buffer.
 * \param[in] dma_r_ptr_get Callback to get the DMA read pointer for the TX buffer.
 * \param[in] tx_dma_start_cb Callback to start the DMA transfer for TX, this function will be called when data is
 *                            placed in \p tx_dma_buffer. The callback function must start the DMA transfer of that
 *                            buffer with the size provided in the callback.
 */
void uart_driver_init(uart_driver_ctx_t *uart_driver, volatile uint8_t *rx_buff, uint8_t rx_buff_size, uint8_t *tx_buff,
                      uint8_t tx_buff_size, volatile uint8_t *tx_dma_buffer, size_t tx_dma_buffer_size,
                      dma_rw_ptr_get_cb dma_w_ptr_get, uart_tx_dma_start_cb tx_dma_start_cb);
/**
 * \brief Read data from the UART driver.
 *
 * \param[inout] uart_driver The UART driver.
 * \param[out] data The data read from the driver.
 * \param[in] size The number of bytes to read.
 * \param[inout] checksum A running checksum of the received data. May be NULL to omit.
 *
 * \return The number of bytes read.
 */
uint8_t uart_driver_read(uart_driver_ctx_t *uart_driver, uint8_t *data, uint8_t size, uint8_t *checksum);

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
 * \param[inout] checksum A running checksum of the data to be written. May be NULL to omit.
 *
 * \return The number of bytes written.
 */
uint8_t uart_driver_write(uart_driver_ctx_t *uart_driver, uint8_t *data, uint8_t size, uint8_t *checksum);

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

/**
 * \brief Callback function to handle the completion of a TX DMA transfer.
 *
 * Call this function when the DMA transfer for TX is complete.
 */
void uart_driver_tx_dma_transfer_complete(uart_driver_ctx_t *uart_driver);