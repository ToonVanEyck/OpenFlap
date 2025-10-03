#define _GNU_SOURCE
#include "chain_comm_node_test.h"

#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// static void *cc_test_node_thread_loop(void *arg);
static void *cc_test_node_thread_loop(void *arg);
static bool node_set_handler(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size);
static bool node_get_handler(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size);

bool cc_test_node_init(cc_test_node_group_ctx_t *node_test_grp, size_t node_cnt, cc_test_master_ctx_t *master)
{
    const cc_node_uart_cb_cfg_t uart_cb = {
        .read          = (uart_read_cb_t)uart_read,
        .cnt_readable  = (uart_cnt_readable_cb_t)uart_cnt_readable,
        .write         = (uart_write_cb_t)uart_write,
        .cnt_writable  = (uart_cnt_writable_cb_t)uart_cnt_writable,
        .tx_buff_empty = (uart_tx_buff_empty_cb_t)uart_tx_buff_empty,
        .is_busy       = (uart_is_busy_cb_t)uart_is_busy,
    };

    printf("Initializing %zu nodes\n", node_cnt);
    node_test_grp->node_cnt = node_cnt;
    node_test_grp->running  = true;

    /* Create the node list. */
    node_test_grp->node_list = malloc(node_cnt * sizeof(cc_test_node_ctx_t));
    if (!node_test_grp->node_list) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    memset(node_test_grp->node_list, 0, node_cnt * sizeof(cc_test_node_ctx_t));
    for (size_t i = 0; i < node_cnt; i++) {
        node_test_grp->node_list[i].id = i;

        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        node_test_grp->node_list[i].uart.rx_fd     = -1; // This will be set when connecting masters
        node_test_grp->node_list[i].uart.tx_fd     = pipefd[1];
        node_test_grp->node_list[i].original_rx_fd = pipefd[0];
        memset(node_test_grp->node_list[i].node_data, 0, TEST_PROP_SIZE);

        asprintf(&node_test_grp->node_list[i].uart.print_debug, "NODE %zu", i);
        uart_delay_xth_tx(&node_test_grp->node_list[i].uart, 0, 0); // No delay by default

        cc_node_init(&node_test_grp->node_list[i].node_ctx, &uart_cb, &node_test_grp->node_list[i].uart, cc_prop_list,
                     PROPERTY_CNT, &node_test_grp->node_list[i]);
    }

    /* Connect the master and nodes. */
    node_test_grp->node_list[0].uart.rx_fd = master->original_rx_fd;

    for (size_t i = 1; i < node_cnt; i++) {
        node_test_grp->node_list[i].uart.rx_fd = node_test_grp->node_list[i - 1].original_rx_fd;
    }

    master->uart.rx_fd = node_test_grp->node_list[node_cnt - 1].original_rx_fd;

    /* Start the node thread. */
    if (pthread_create(&node_test_grp->thread, NULL, cc_test_node_thread_loop, node_test_grp) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    usleep(50000); // Wait a bit for threads to start
    return true;
}

bool cc_test_node_deinit(cc_test_node_group_ctx_t *node_test_grp)
{
    node_test_grp->running = false;
    pthread_join(node_test_grp->thread, NULL);
    for (int i = 0; i < node_test_grp->node_cnt; i++) {
        close(node_test_grp->node_list[i].uart.rx_fd);
        close(node_test_grp->node_list[i].uart.tx_fd);
        free(node_test_grp->node_list[i].uart.print_debug);
    }
    free(node_test_grp->node_list);
    return true;
}

void setup_cc_node_prop_list_handlers(void)
{
    cc_prop_list[PROP_STATIC_RW].handler.get = node_get_handler;
    cc_prop_list[PROP_STATIC_RW].handler.set = node_set_handler;

    cc_prop_list[PROP_STATIC_RO].handler.get = node_get_handler;
    cc_prop_list[PROP_STATIC_RO].handler.set = NULL;

    cc_prop_list[PROP_STATIC_WO].handler.get = NULL;
    cc_prop_list[PROP_STATIC_WO].handler.set = node_set_handler;

    cc_prop_list[PROP_DYNAMIC_RW].handler.get = node_get_handler;
    cc_prop_list[PROP_DYNAMIC_RW].handler.set = node_set_handler;

    cc_prop_list[PROP_DYNAMIC_RO].handler.get = node_get_handler;
    cc_prop_list[PROP_DYNAMIC_RO].handler.set = NULL;

    cc_prop_list[PROP_DYNAMIC_WO].handler.get = NULL;
    cc_prop_list[PROP_DYNAMIC_WO].handler.set = node_set_handler;
}

static void *cc_test_node_thread_loop(void *arg)
{
    cc_test_node_group_ctx_t *node_test_grp = (cc_test_node_group_ctx_t *)arg;

    struct timespec ts;
    size_t ctx_idx = 0;
    /* Loop over all nodes. */
    while (node_test_grp->running) {
        // Get current timestamp in milliseconds
        clock_gettime(CLOCK_MONOTONIC, &ts);
        uint64_t ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
        cc_node_tick(&node_test_grp->node_list[ctx_idx].node_ctx, ms);
        ctx_idx = (ctx_idx + 1) % node_test_grp->node_cnt;
        if (ctx_idx == 0) {
            sched_yield();
        }
    }
    printf("Node thread exiting\n");
    return NULL;
}

static bool node_set_handler(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    (void)node_idx;
    cc_test_node_ctx_t *ctx = (cc_test_node_ctx_t *)userdata;
    memcpy(ctx->node_data, buf, *size);
    for (uint16_t i = 0; i < *size; i++) {
        printf("  Data[%d]: %02X\n", i, buf[i]);
    }
    return true;
}

static bool node_get_handler(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    printf("Dummy node get handler called for node %d\n", node_idx);
    cc_test_node_ctx_t *ctx = (cc_test_node_ctx_t *)userdata;
    memcpy(buf, ctx->node_data, TEST_PROP_SIZE);
    *size = TEST_PROP_SIZE;
    return true;
}
