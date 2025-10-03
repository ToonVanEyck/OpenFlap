#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    int rx_fd; /* Receive file descriptor */
    int tx_fd; /* Transmit file descriptor */
    uint32_t read_timeout_ms;
    uint32_t tx_delay_xth; /* Introduce a delay after every xth byte transmitted. 0 = no delay. Counts down to 0. */
    uint32_t tx_delay_ms;  /* Delay in milliseconds. */
    char *print_debug;     /* If true, print debug information. */
} uart_driver_t;

void test_uart_init(uart_driver_t *ctx);

void uart_read_timeout_set(uart_driver_t *ctx, uint32_t timeout_ms);
ssize_t uart_read(uart_driver_t *ctx, uint8_t *data, size_t size);
ssize_t uart_cnt_readable(uart_driver_t *ctx);
ssize_t uart_write(uart_driver_t *ctx, const uint8_t *data, size_t size);
ssize_t uart_cnt_writable(uart_driver_t *ctx);
bool uart_tx_buff_empty(uart_driver_t *ctx);
bool uart_is_busy(uart_driver_t *ctx);
void uart_flush_rx_buff(uart_driver_t *ctx);

/**
 * \brief Introduce a delay after every xth byte transmitted.
 * \param ctx Pointer to the uart_driver_t structure.
 * \param xth Introduce a delay after every xth byte transmitted. 0 = no delay.
 * \param delay_ms Delay in milliseconds.
 */
void uart_delay_xth_tx(uart_driver_t *ctx, size_t xth, uint32_t delay_ms);