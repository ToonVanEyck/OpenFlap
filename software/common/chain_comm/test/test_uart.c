#define _GNU_SOURCE
#include "test_uart.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static int pipe_capacity(int fd);
static int pipe_used(int fd);

void uart_read_timeout_set(uart_driver_t *ctx, uint32_t timeout_ms)
{
    if (ctx) {
        ctx->read_timeout_ms = timeout_ms;
    }
}

ssize_t uart_read(uart_driver_t *ctx, uint8_t *data, size_t size)
{
    if (!ctx || !data || size == 0) {
        errno = EINVAL;
        return -1;
    }
    const int timeout_ms = ctx->read_timeout_ms;
    struct timespec start;
    if (timeout_ms > 0 && clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
        return -1;
    }

    size_t bytes_read = 0;

    while (bytes_read < size) {
        int remaining = timeout_ms;

        if (timeout_ms > 0) {
            struct timespec now;
            if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
                return -1;
            }
            long sec  = now.tv_sec - start.tv_sec;
            long nsec = now.tv_nsec - start.tv_nsec;
            if (nsec < 0) {
                sec--;
                nsec += 1000000000L;
            }
            long elapsed = sec * 1000L + nsec / 1000000L;
            remaining    = timeout_ms - (int)elapsed;
            if (remaining <= 0) {
                break;
            }
        }

        struct pollfd pfd = {.fd = ctx->rx_fd, .events = POLLIN};
        int rv            = poll(&pfd, 1, remaining);
        if (rv == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        } else if (rv == 0) {
            break; // timeout expired
        }

        size_t r = read(ctx->rx_fd, data + bytes_read, size - bytes_read);
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        } else if (r == 0) {
            break; // EOF
        }

        bytes_read += r;

        if (timeout_ms == 0) {
            // Non-blocking mode: read once, return immediately
            break;
        }
    }

    if (ctx->print_debug) {
        for (size_t i = 0; i < bytes_read; i++) {
            printf("[%s] RX: %02X\n", ctx->print_debug, data[i]);
        }
        if (bytes_read == 0 && timeout_ms > 0) {
            printf("[%s] RX: timeout after %u ms\n", ctx->print_debug, timeout_ms);
        }
    }
    return (size_t)bytes_read;
}

ssize_t uart_cnt_readable(uart_driver_t *ctx)
{
    if (!ctx) {
        return 0;
    }
    return pipe_used(ctx->rx_fd);
}

ssize_t uart_write(uart_driver_t *ctx, const uint8_t *data, size_t size)
{
    if (!ctx || !data || size == 0) {
        return 0;
    }
    // return write(ctx->tx_fd, data, size);
    if (ctx->tx_delay_xth == 0) {
        if (ctx->tx_delay_ms > 0) {
            if (ctx->print_debug) {
                printf("[%s] TX: delay %u ms ...\n", ctx->print_debug, ctx->tx_delay_ms);
            }
            usleep(ctx->tx_delay_ms * 1000);
            ctx->tx_delay_ms = 0; // Reset to 0 to disable further delays
        }
        ssize_t w = write(ctx->tx_fd, data, size);
        if (w < 0 && ctx->print_debug) {
            printf("[%s] TX: failed\n", ctx->print_debug);
        } else {
            for (size_t i = 0; i < w && ctx->print_debug; i++) {
                printf("[%s] TX: %02X\n", ctx->print_debug, data[i]);
            }
        }
        return w;
    } else if (ctx->tx_delay_xth > size) {
        ssize_t w = write(ctx->tx_fd, data, size);
        if (w > 0) {
            ctx->tx_delay_xth -= w;
            for (size_t i = 0; i < w && ctx->print_debug; i++) {
                printf("[%s] TX: %02X\n", ctx->print_debug, data[i]);
            }
        }
        return w;
    } else {
        size_t no_delay_cnt = size < ctx->tx_delay_xth ? size : ctx->tx_delay_xth;
        ssize_t w           = write(ctx->tx_fd, data, no_delay_cnt);
        if (w < 0) {
            if (ctx->print_debug) {
                printf("[%s] TX: failed\n", ctx->print_debug);
            }
            return w; // Write error
        }
        for (size_t i = 0; i < w && ctx->print_debug; i++) {
            printf("[%s] TX: %02X\n", ctx->print_debug, data[i]);
        }
        ctx->tx_delay_xth -= w;
        if (ctx->print_debug) {
            printf("[%s] TX: delay %u ms ...\n", ctx->print_debug, ctx->tx_delay_ms);
        }
        usleep(ctx->tx_delay_ms * 1000);
        ctx->tx_delay_ms = 0; // Reset to 0 to disable further delays
        ssize_t w2       = write(ctx->tx_fd, data + w, size - w);
        if (w2) {
            for (size_t i = 0; i < w2 && ctx->print_debug; i++) {
                printf("[%s] TX: %02X\n", ctx->print_debug, data[w + i]);
            }
            w += w2; // Write error
        }
        return w;
    }
}

ssize_t uart_cnt_writable(uart_driver_t *ctx)
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

static int pipe_capacity(int fd)
{
    int size = fcntl(fd, F_GETPIPE_SZ);
    if (size == -1) {
        perror("fcntl(F_GETPIPE_SZ)");
        exit(EXIT_FAILURE);
    }
    return size;
}

static int pipe_used(int fd)
{
    int count;
    if (ioctl(fd, FIONREAD, &count) == -1) {
        perror("ioctl(FIONREAD)");
        exit(EXIT_FAILURE);
    }
    return count;
}

void uart_delay_xth_tx(uart_driver_t *ctx, size_t xth, uint32_t delay_ms)
{
    if (!ctx) {
        return;
    }
    ctx->tx_delay_xth = xth;
    ctx->tx_delay_ms  = delay_ms;
}

void uart_flush_rx_buff(uart_driver_t *ctx)
{
    if (!ctx) {
        return;
    }
    uint8_t buf[256];
    while (pipe_used(ctx->rx_fd) > 0) {
        ssize_t r = read(ctx->rx_fd, buf, sizeof(buf));
        if (r <= 0) {
            break;
        }
        if (ctx->print_debug) {
            for (ssize_t i = 0; i < r; i++) {
                printf("[%s] RX: flushed %02X\n", ctx->print_debug, buf[i]);
            }
        }
    }
    return;
}