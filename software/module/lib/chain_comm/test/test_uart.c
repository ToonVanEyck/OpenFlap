#define _GNU_SOURCE
#include "test_uart.h"

#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

static int pipe_capacity(int fd);
static int pipe_used(int fd);

void uart_read_timeout_set(uart_driver_t *ctx, uint32_t timeout_ms)
{
    if (ctx) {
        ctx->read_timeout_ms = timeout_ms;
    }
}

size_t uart_read(uart_driver_t *ctx, uint8_t *data, size_t size)
{
    if (!ctx || !data || size == 0) {
        return 0;
    }
    struct pollfd pfd = {.fd = ctx->rx_fd, .events = POLLIN};
    int rv            = poll(&pfd, 1, ctx->read_timeout_ms);

    if (rv == -1) {
        perror("poll");
        return -1;
    } else if (rv == 0) {
        return 0; // timeout
    } else {
        return read(ctx->rx_fd, data, size);
    }
}

size_t uart_cnt_readable(uart_driver_t *ctx)
{
    if (!ctx) {
        return 0;
    }
    return pipe_used(ctx->rx_fd);
}

size_t uart_write(uart_driver_t *ctx, const uint8_t *data, size_t size)
{
    if (!ctx || !data || size == 0) {
        return 0;
    }
    return write(ctx->tx_fd, data, size);
}

size_t uart_cnt_writable(uart_driver_t *ctx)
{
    if (!ctx) {
        return 0;
    }
    return pipe_capacity(ctx->tx_fd) - pipe_used(ctx->tx_fd);
}

bool uart_tx_buff_empty(uart_driver_t *ctx)
{
    if (!ctx) {
        return 1;
    }
    return pipe_used(ctx->tx_fd) == 0;
}

bool uart_is_busy(uart_driver_t *ctx)
{
    if (!ctx) {
        return false;
    }
    return pipe_used(ctx->tx_fd) || pipe_used(ctx->rx_fd);
}

//-----------------------------------------
static int pipe_capacity(int fd)
{
    int size = fcntl(fd, F_GETPIPE_SZ);
    if (size == -1) {
        perror("fcntl(F_GETPIPE_SZ)");
        exit(EXIT_FAILURE);
    }
    return size;
}
//-----------------------------------------
static int pipe_used(int fd)
{
    int count;
    if (ioctl(fd, FIONREAD, &count) == -1) {
        perror("ioctl(FIONREAD)");
        exit(EXIT_FAILURE);
    }
    return count;
}