#include "uart_driver.h"

void uart_driver_update_tx_dma_buffer(uart_driver_ctx_t *uart_driver);

void uart_driver_init(uart_driver_ctx_t *uart_driver, volatile uint8_t *rx_buff, uint8_t rx_buff_size, uint8_t *tx_buff,
                      uint8_t tx_buff_size, volatile uint8_t *tx_dma_buffer, size_t tx_dma_buffer_size,
                      dma_rw_ptr_get_cb dma_w_ptr_get, uart_tx_dma_start_cb tx_dma_start_cb)
{
    rbuff_init_dma_ro(&uart_driver->rx_rbuff, rx_buff, rx_buff_size, sizeof(uint8_t), dma_w_ptr_get);
    rbuff_init(&uart_driver->tx_rbuff, tx_buff, tx_buff_size, sizeof(uint8_t));
    uart_driver->tx_dma_busy        = false;
    uart_driver->tx_dma_buffer      = tx_dma_buffer;
    uart_driver->tx_dma_buffer_size = tx_dma_buffer_size;
    uart_driver->tx_dma_start_cb    = tx_dma_start_cb;
}

uint8_t uart_driver_read(uart_driver_ctx_t *uart_driver, uint8_t *data, uint8_t size, uint8_t *checksum)
{
    uint8_t rx_cnt = rbuff_read(&uart_driver->rx_rbuff, data, size);
    for (uint8_t i = 0; checksum && i < rx_cnt; i++) {
        *checksum += data[i];
    }
    return rx_cnt;
}

uint8_t uart_driver_cnt_readable(uart_driver_ctx_t *uart_driver)
{
    return rbuff_cnt_used(&uart_driver->rx_rbuff);
}

uint8_t uart_driver_write(uart_driver_ctx_t *uart_driver, uint8_t *data, uint8_t size, uint8_t *checksum)
{
    uint8_t tx_cnt = rbuff_write(&uart_driver->tx_rbuff, data, size);
    for (uint8_t i = 0; checksum && i < tx_cnt; i++) {
        *checksum += data[i];
    }
    uart_driver_update_tx_dma_buffer(uart_driver);
    return tx_cnt;
}

uint8_t uart_driver_cnt_writable(uart_driver_ctx_t *uart_driver)
{
    return rbuff_cnt_free(&uart_driver->tx_rbuff);
}

uint8_t uart_driver_cnt_written(uart_driver_ctx_t *uart_driver)
{
    return rbuff_cnt_used(&uart_driver->tx_rbuff);
}

bool uart_driver_rx_in_progress(uart_driver_ctx_t *uart_driver)
{
    return uart_driver_cnt_readable(uart_driver) > 0;
}

bool uart_driver_tx_in_progress(uart_driver_ctx_t *uart_driver)
{
    return uart_driver_cnt_written(uart_driver) > 0;
}

bool uart_driver_is_busy(uart_driver_ctx_t *uart_driver)
{
    return uart_driver_rx_in_progress(uart_driver) || uart_driver_tx_in_progress(uart_driver);
}

void uart_driver_tx_dma_transfer_complete(uart_driver_ctx_t *uart_driver)
{
    uart_driver->tx_dma_busy = false;
    uart_driver_update_tx_dma_buffer(uart_driver);
}

void uart_driver_update_tx_dma_buffer(uart_driver_ctx_t *uart_driver)
{
    if (uart_driver->tx_dma_busy) {
        return; // Do not update if DMA is busy
    }
    size_t bytes_to_write =
        rbuff_read(&uart_driver->tx_rbuff, (void *)uart_driver->tx_dma_buffer, uart_driver->tx_dma_buffer_size);
    if (bytes_to_write > 0) {
        uart_driver->tx_dma_busy = true;
        uart_driver->tx_dma_start_cb(bytes_to_write);
    }
}