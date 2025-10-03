#pragma once

#include "chain_comm_master.h"
#include "test_uart.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    int original_rx_fd; // Original read end of the pipe;
    uart_driver_t uart;
    cc_master_ctx_t master_ctx;
    uint8_t *node_data;
    size_t node_cnt;
} cc_test_master_ctx_t;

void cc_test_master_init(cc_test_master_ctx_t *ctx);
void cc_test_master_deinit(cc_test_master_ctx_t *ctx);

void setup_cc_master_prop_list_handlers(void);