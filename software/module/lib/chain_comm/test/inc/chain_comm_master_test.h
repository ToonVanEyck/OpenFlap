#pragma once

#include "chain_comm_master.h"
#include "test_uart.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    int original_tx_fd; // Original write end of the pipe;
    uart_driver_t uart;
    cc_master_ctx_t master_ctx;
} cc_test_master_ctx_t;

bool cc_test_master_init(cc_test_master_ctx_t *ctx);
bool cc_test_master_deinit(cc_test_master_ctx_t *ctx);