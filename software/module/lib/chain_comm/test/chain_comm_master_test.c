#include "chain_comm_master_test.h"
#include "test_properties.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool node_cnt_update(void *userdata, uint16_t node_cnt);
static bool node_exists_and_must_be_written(void *userdata, uint16_t node_idx, uint8_t property, bool *must_be_written);
static void master_set_handler(uint16_t node_idx, uint8_t *buf, uint16_t *size, void *userdata);
static void master_get_handler(uint16_t node_idx, uint8_t *buf, uint16_t *size, void *userdata);

void cc_test_master_init(cc_test_master_ctx_t *ctx)
{
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    ctx->uart.rx_fd     = -1; // This will be set when connecting masters
    ctx->uart.tx_fd     = pipefd[1];
    ctx->original_rx_fd = pipefd[0];
    ctx->node_data      = NULL;
    ctx->node_cnt       = 0;

    ctx->uart.print_debug = "MASTER";

    cc_master_uart_cb_cfg_t uart_cb = {
        .read             = (uart_read_cb_t)uart_read,
        .cnt_readable     = (uart_cnt_readable_cb_t)uart_cnt_readable,
        .write            = (uart_write_cb_t)uart_write,
        .read_timeout_set = (uart_read_timeout_set_cb_t)uart_read_timeout_set,
    };

    uart_delay_xth_tx(&ctx->uart, 0, 0); // No delay by default

    cc_master_cb_cfg_t master_cb = {
        .node_cnt_update = (cc_master_node_cnt_update_cb_t)node_cnt_update,
        .node_exists_and_must_be_written =
            (cc_master_node_exists_and_must_be_written_cb_t)node_exists_and_must_be_written,
    };

    setup_cc_master_property_list_handlers();
    cc_prop_t *master_properties = calloc(PROPERTY_CNT, sizeof(cc_prop_t));
    memcpy(master_properties, cc_property_list, PROPERTY_CNT * sizeof(cc_prop_t));

    cc_master_init(&ctx->master_ctx, &uart_cb, &ctx->uart, &master_cb, master_properties, PROPERTY_CNT, ctx);

    return;
}

void cc_test_master_deinit(cc_test_master_ctx_t *ctx)
{
    free(ctx->node_data);
    free(ctx->master_ctx.property_list);
    return;
}

void setup_cc_master_property_list_handlers(void)
{
    cc_property_list[PROP_STATIC_RW - 1].handler.get = master_get_handler;
    cc_property_list[PROP_STATIC_RW - 1].handler.set = master_set_handler;

    cc_property_list[PROP_STATIC_RO - 1].handler.get = NULL;
    cc_property_list[PROP_STATIC_RO - 1].handler.set = master_set_handler;

    cc_property_list[PROP_STATIC_WO - 1].handler.get = master_get_handler;
    cc_property_list[PROP_STATIC_WO - 1].handler.set = NULL;

    cc_property_list[PROP_DYNAMIC_RW - 1].handler.get = master_get_handler;
    cc_property_list[PROP_DYNAMIC_RW - 1].handler.set = master_set_handler;

    cc_property_list[PROP_DYNAMIC_RO - 1].handler.get = NULL;
    cc_property_list[PROP_DYNAMIC_RO - 1].handler.set = master_set_handler;

    cc_property_list[PROP_DYNAMIC_WO - 1].handler.get = master_get_handler;
    cc_property_list[PROP_DYNAMIC_WO - 1].handler.set = NULL;
}

static bool node_cnt_update(void *userdata, uint16_t node_cnt)
{
    cc_test_master_ctx_t *ctx = (cc_test_master_ctx_t *)userdata;
    printf("Node count updated to %d (%p)\n", node_cnt, ctx);

    ctx->node_cnt  = node_cnt;
    ctx->node_data = realloc(ctx->node_data, node_cnt * TEST_PROP_SIZE);
    if (!ctx->node_data) {
        perror("realloc");
        return false;
    }

    return true;
}

static bool node_exists_and_must_be_written(void *userdata, uint16_t node_idx, uint8_t property, bool *must_be_written)
{
    cc_test_master_ctx_t *ctx = (cc_test_master_ctx_t *)userdata;

    *must_be_written = node_idx % 2 == 0; // Write to even nodes only
    bool exists      = node_idx < ctx->node_cnt;
    // printf("Node %d exists: %d, must be written: %d (%p)\n", node_idx, exists, *must_be_written, ctx);
    return exists;
}

static void master_set_handler(uint16_t node_idx, uint8_t *buf, uint16_t *size, void *userdata)
{
    cc_test_master_ctx_t *ctx = (cc_test_master_ctx_t *)userdata;
    memcpy(ctx->node_data + node_idx * TEST_PROP_SIZE, buf, *size);
    printf("Dummy set handler called for node %d with size %d and user data %p:\n", node_idx, *size, userdata);
    for (uint16_t i = 0; i < *size; i++) {
        printf("  Data[%d]: %02X\n", i, buf[i]);
    }
}

static void master_get_handler(uint16_t node_idx, uint8_t *buf, uint16_t *size, void *userdata)
{
    // printf("Dummy get handler called for node %d with size %d and user data %p\n", node_idx, *size, userdata);
    cc_test_master_ctx_t *ctx = (cc_test_master_ctx_t *)userdata;
    *size                     = TEST_PROP_SIZE;
    memcpy(buf, ctx->node_data + node_idx * TEST_PROP_SIZE, *size);
}