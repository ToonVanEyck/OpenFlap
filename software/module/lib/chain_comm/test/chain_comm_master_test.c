#include "chain_comm_master_test.h"
#include "test_master_properties.h"

#include <unistd.h>

static bool node_cnt_update(void *userdata, uint16_t node_cnt);
static bool node_exists_and_must_be_written(void *userdata, uint16_t node_idx, uint8_t property, bool *must_be_written);

bool cc_test_master_init(cc_test_master_ctx_t *ctx)
{

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return false;
    }

    ctx->uart.rx_fd     = pipefd[0];
    ctx->uart.tx_fd     = -1; // This will be set when connecting masters
    ctx->original_tx_fd = pipefd[1];

    cc_master_uart_cb_cfg_t uart_cb = {
        .read             = (uart_read_cb_t)uart_read,
        .cnt_readable     = (uart_cnt_readable_cb_t)uart_cnt_readable,
        .write            = (uart_write_cb_t)uart_write,
        .read_timeout_set = (uart_read_timeout_set_cb_t)uart_read_timeout_set,
    };

    cc_master_cb_cfg_t master_cb = {
        .node_cnt_update = (cc_master_node_cnt_update_cb_t)node_cnt_update,
        .node_exists_and_must_be_written =
            (cc_master_node_exists_and_must_be_written_cb_t)node_exists_and_must_be_written,
    };

    cc_master_init(&ctx->master_ctx, &uart_cb, &ctx->uart, &master_cb, ctx);

    return true;
}

bool cc_test_master_deinit(cc_test_master_ctx_t *ctx)
{
    return true;
}

static bool node_cnt_update(void *userdata, uint16_t node_cnt)
{
    cc_test_master_ctx_t *ctx = (cc_test_master_ctx_t *)userdata;
    return true;
}

static bool node_exists_and_must_be_written(void *userdata, uint16_t node_idx, uint8_t property, bool *must_be_written)
{
    cc_test_master_ctx_t *ctx = (cc_test_master_ctx_t *)userdata;
    return true;
}
