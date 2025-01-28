#include "uart_driver.h"
#include "debug_io.h"

void uart_driver_init(uart_driver_ctx_t *uart_driver, UART_HandleTypeDef *huart, uint8_t *rx_buff, uint8_t rx_buff_size,
                      uint8_t *tx_buff, uint8_t tx_buff_size)
{
    uart_driver->huart = huart;
    rbuff_init(&uart_driver->rx_rbuff, rx_buff, rx_buff_size);
    rbuff_init(&uart_driver->tx_rbuff, tx_buff, tx_buff_size);
    HAL_UART_Receive_IT(uart_driver->huart, &uart_driver->rx_tmp_buff, 1);
}

void uart_driver_rx_isr(uart_driver_ctx_t *uart_driver)
{
    if (rbuff_write(&uart_driver->rx_rbuff, &uart_driver->rx_tmp_buff, 1) == 0) {
        debug_io_log_error("RX buffer overflow\n");
    }
    HAL_UART_Receive_IT(uart_driver->huart, &uart_driver->rx_tmp_buff, 1);
}

void uart_driver_ctx_tx_isr(uart_driver_ctx_t *uart_driver)
{
    if (uart_driver->huart->gState == HAL_UART_STATE_READY && uart_driver->huart->Lock == HAL_UNLOCKED &&
        rbuff_read(&uart_driver->tx_rbuff, &uart_driver->tx_tmp_buff, 1)) {
        if (HAL_UART_Transmit_IT(uart_driver->huart, &uart_driver->tx_tmp_buff, 1) != HAL_OK) {
            debug_io_log_error("TX error\n");
        }
    }
}

uint8_t uart_driver_read(uart_driver_ctx_t *uart_driver, uint8_t *data, uint8_t size)
{
    HAL_UART_Receive_IT(uart_driver->huart, &uart_driver->rx_tmp_buff, 1);
    uint8_t rx_cnt = rbuff_read(&uart_driver->rx_rbuff, data, size);
    for (uint8_t i = 0; i < rx_cnt; i++) {
        debug_io_log_debug("RX : 0x%02X\n", data[i]);
    }
    return rx_cnt;
}

uint8_t uart_driver_cnt_readable(uart_driver_ctx_t *uart_driver)
{
    return rbuff_cnt_used(&uart_driver->rx_rbuff);
}

uint8_t uart_driver_write(uart_driver_ctx_t *uart_driver, uint8_t *data, uint8_t size)
{
    uint8_t tx_cnt = rbuff_write(&uart_driver->tx_rbuff, data, size);
    for (uint8_t i = 0; i < tx_cnt; i++) {
        debug_io_log_debug("TX : 0x%02X\n", data[i]);
    }
    uart_driver_ctx_tx_isr(uart_driver);
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

bool uart_driver_is_busy(uart_driver_ctx_t *uart_driver)
{
    return (!rbuff_is_empty(&uart_driver->rx_rbuff)) && (!rbuff_is_empty(&uart_driver->tx_rbuff));
}
