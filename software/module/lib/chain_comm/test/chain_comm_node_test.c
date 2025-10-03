#include "chain_comm_node_test.h"

#include <pthread.h>
#include <string.h>
#include <unistd.h>

static void *cc_test_node_thread_loop(void *arg);

bool cc_test_node_init(size_t id, cc_test_node_ctx_t *ctx)
{
    ctx->id      = id;
    ctx->running = true;
    if (pthread_create(&ctx->thread, NULL, cc_test_node_thread_loop, ctx) != 0) {
        perror("pthread_create");
        return false;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return false;
    }

    ctx->uart.rx_fd     = -1; // This will be set when connecting masters
    ctx->uart.tx_fd     = pipefd[1];
    ctx->original_rx_fd = pipefd[0];

    cc_node_uart_cb_cfg_t uart_cb = {
        .read          = (uart_read_cb_t)uart_read,
        .cnt_readable  = (uart_cnt_readable_cb_t)uart_cnt_readable,
        .write         = (uart_write_cb_t)uart_write,
        .cnt_writable  = (uart_cnt_writable_cb_t)uart_cnt_writable,
        .tx_buff_empty = (uart_tx_buff_empty_cb_t)uart_tx_buff_empty,
        .is_busy       = (uart_is_busy_cb_t)uart_is_busy,
    };

    cc_node_init(&ctx->node_ctx, &uart_cb, &ctx->uart, cc_property_list, PROPERTY_CNT);

    return true;
}

bool cc_test_node_deinit(cc_test_node_ctx_t *ctx)
{
    ctx->running = false;
    pthread_join(ctx->thread, NULL);
    return true;
}

static void *cc_test_node_thread_loop(void *arg)
{
    cc_test_node_ctx_t *ctx = (cc_test_node_ctx_t *)arg;
    while (ctx->running && ctx->uart.rx_fd == -1) {
        usleep(1000);
    }
    printf("Node %ld thread started\n", ctx->id);
    while (ctx->running) {
        // Get current timestamp in milliseconds
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        uint64_t ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

        cc_node_tick(&ctx->node_ctx, ms);
        usleep(25); // Sleep for 25us to avoid busy loop
    }
    printf("Node %ld thread stopped\n", ctx->id);
    return NULL;
}