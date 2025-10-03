#pragma once

#include "chain_comm_master_test.h"
#include "chain_comm_node.h"
#include "test_properties.h"
#include "test_uart.h"

#include <pthread.h>
#include <stdbool.h>

typedef struct {
    size_t id;
    cc_node_ctx_t node_ctx;
    uart_driver_t uart;
    int original_rx_fd; /* This fd must be passed to the node before this one. */
    uint8_t node_data[TEST_PROP_SIZE];
} cc_test_node_ctx_t;

typedef struct {
    size_t node_cnt;
    cc_test_node_ctx_t *node_list;
    pthread_t thread;
    bool running;
} cc_test_node_group_ctx_t;

bool cc_test_node_init(cc_test_node_group_ctx_t *node_test_grp, size_t node_cnt, cc_test_master_ctx_t *master);

bool cc_test_node_deinit(cc_test_node_group_ctx_t *node_test_grp);

void setup_cc_node_prop_list_handlers(void);