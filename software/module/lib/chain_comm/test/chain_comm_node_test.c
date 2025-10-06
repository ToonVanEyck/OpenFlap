#include "chain_comm_node_test.h"

#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <unistd.h>

static void *cc_test_node_thread_loop(void *arg);
static void node_set_handler(uint16_t node_idx, uint8_t *buf, uint16_t *size, void *userdata);
static void node_get_handler(uint16_t node_idx, uint8_t *buf, uint16_t *size, void *userdata);

bool cc_test_node_init(size_t id, cc_test_node_ctx_t *ctx)
{
    ctx->id      = id;
    ctx->running = true;

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return false;
    }

    ctx->uart.rx_fd     = -1; // This will be set when connecting masters
    ctx->uart.tx_fd     = pipefd[1];
    ctx->original_rx_fd = pipefd[0];

    memset(ctx->node_data, 0, TEST_PROP_SIZE);

    const cc_node_uart_cb_cfg_t uart_cb = {
        .read          = (uart_read_cb_t)uart_read,
        .cnt_readable  = (uart_cnt_readable_cb_t)uart_cnt_readable,
        .write         = (uart_write_cb_t)uart_write,
        .cnt_writable  = (uart_cnt_writable_cb_t)uart_cnt_writable,
        .tx_buff_empty = (uart_tx_buff_empty_cb_t)uart_tx_buff_empty,
        .is_busy       = (uart_is_busy_cb_t)uart_is_busy,
    };

    cc_node_init(&ctx->node_ctx, &uart_cb, &ctx->uart, cc_property_list, PROPERTY_CNT, ctx);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 254 * 1024); // 1 MB per thread
    if (pthread_create(&ctx->thread, &attr, cc_test_node_thread_loop, ctx) != 0) {
        perror("pthread_create");
        return false;
    }
    pthread_attr_destroy(&attr);

    return true;
}

bool cc_test_node_deinit(cc_test_node_ctx_t *ctx)
{

    /* Inspect the actual thread attributes for the running thread (GNU extension) */
    pthread_attr_t attr;
    if (pthread_getattr_np(ctx->thread, &attr) == 0) {
        size_t actual_stack = 0;
        pthread_attr_getstacksize(&attr, &actual_stack);
        printf("Actual thread stack size: %zu bytes (%.1f KB)\n", actual_stack, actual_stack / 1024.0);
        pthread_attr_destroy(&attr);
    } else {
        perror("pthread_getattr_np");
    }

    ctx->running = false;
    pthread_join(ctx->thread, NULL);
    return true;
}

void setup_cc_node_property_list_handlers(void)
{
    cc_property_list[PROP_STATIC_RW - 1].handler.get = node_get_handler;
    cc_property_list[PROP_STATIC_RW - 1].handler.set = node_set_handler;

    cc_property_list[PROP_STATIC_RO - 1].handler.get = NULL;
    cc_property_list[PROP_STATIC_RO - 1].handler.set = node_set_handler;

    cc_property_list[PROP_STATIC_WO - 1].handler.get = node_get_handler;
    cc_property_list[PROP_STATIC_WO - 1].handler.set = NULL;

    cc_property_list[PROP_DYNAMIC_RW - 1].handler.get = node_get_handler;
    cc_property_list[PROP_DYNAMIC_RW - 1].handler.set = node_set_handler;

    cc_property_list[PROP_DYNAMIC_RO - 1].handler.get = NULL;
    cc_property_list[PROP_DYNAMIC_RO - 1].handler.set = node_set_handler;

    cc_property_list[PROP_DYNAMIC_WO - 1].handler.get = node_get_handler;
    cc_property_list[PROP_DYNAMIC_WO - 1].handler.set = NULL;
}

static void *cc_test_node_thread_loop(void *arg)
{
    cc_test_node_ctx_t *ctx = (cc_test_node_ctx_t *)arg;
    printf("Node %ld waiting on uart connection\n", ctx->id);
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
        sched_yield();
    }
    printf("Node %ld thread stopped\n", ctx->id);
    return NULL;
}

static void node_set_handler(uint16_t node_idx, uint8_t *buf, uint16_t *size, void *userdata)
{
    (void)node_idx;
    printf("Dummy set handler called with size %d and user data %p:\n", *size, userdata);
    cc_test_node_ctx_t *ctx = (cc_test_node_ctx_t *)userdata;
    memcpy(ctx->node_data, buf, *size);
    for (uint16_t i = 0; i < *size; i++) {
        // printf("  Data[%d]: %02X\n", i, buf[i]);
    }
}

static void node_get_handler(uint16_t node_idx, uint8_t *buf, uint16_t *size, void *userdata)
{
    cc_test_node_ctx_t *ctx = (cc_test_node_ctx_t *)userdata;
    memcpy(buf, ctx->node_data, TEST_PROP_SIZE);
    *size = TEST_PROP_SIZE;
}
