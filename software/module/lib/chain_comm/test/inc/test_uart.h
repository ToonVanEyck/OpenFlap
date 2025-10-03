#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    int rx_fd; /* Receive file descriptor */
    int tx_fd; /* Transmit file descriptor */
    uint32_t read_timeout_ms;
} uart_driver_t;

void test_uart_init(uart_driver_t *ctx);

void uart_read_timeout_set(uart_driver_t *ctx, uint32_t timeout_ms);
size_t uart_read(uart_driver_t *ctx, uint8_t *data, size_t size);
size_t uart_cnt_readable(uart_driver_t *ctx);
size_t uart_write(uart_driver_t *ctx, const uint8_t *data, size_t size);
size_t uart_cnt_writable(uart_driver_t *ctx);
bool uart_tx_buff_empty(uart_driver_t *ctx);
bool uart_is_busy(uart_driver_t *ctx);