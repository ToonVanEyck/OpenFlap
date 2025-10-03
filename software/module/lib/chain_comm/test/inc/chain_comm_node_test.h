#pragma once

#include "chain_comm_node.h"
#include "test_uart.h"

#include <pthread.h>
#include <stdbool.h>

typedef struct {
    pthread_t thread;
    bool running;
    size_t id;
    cc_node_ctx_t node_ctx;
    uart_driver_t uart;
    int original_tx_fd; /* This fd must be passed to the node before this one. */
} cc_test_node_ctx_t;

bool cc_test_node_init(size_t id, cc_test_node_ctx_t *ctx);

bool cc_test_node_deinit(cc_test_node_ctx_t *ctx);