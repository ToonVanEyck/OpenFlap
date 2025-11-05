#include "chain_comm_master.h"

#include <assert.h>
#include <string.h>

//======================================================================================================================
//                                                   MACROS a DEFINES
//======================================================================================================================

#define RX_BYTES_TIMEOUT(_byte_cnt) (1000 + (_byte_cnt) * 1) /**< Timeout in ms for receiving bytes. */

#ifndef TAG
#define TAG "chain_comm_master"
#endif

#if !defined(CC_LOGE) || !defined(CC_LOGW) || !defined(CC_LOGI) || !defined(CC_LOGD)
#include <stdio.h>
#define CC_LOGE(tag, format, ...) printf("E: [%s]: " format "\n", tag, ##__VA_ARGS__)
#define CC_LOGW(tag, format, ...) printf("W: [%s]: " format "\n", tag, ##__VA_ARGS__)
#define CC_LOGI(tag, format, ...) printf("I: [%s]: " format "\n", tag, ##__VA_ARGS__)
#define CC_LOGD(tag, format, ...) printf("D: [%s]: " format "\n", tag, ##__VA_ARGS__)
#endif

/**
 * Macro which can be used to check the condition. If the condition is not 'true', it prints the message
 * and returns with the supplied 'err_code'.
 */
#ifndef CC_RETURN_ON_FALSE
#define CC_RETURN_ON_FALSE(a, err_code, log_tag, format, ...)                                                          \
    do {                                                                                                               \
        if (!(a)) {                                                                                                    \
            CC_LOGE(log_tag, "%s(%d): " format, __FUNCTION__, __LINE__ __VA_OPT__(, ) __VA_ARGS__);                    \
            return err_code;                                                                                           \
        }                                                                                                              \
    } while (0)
#endif

/**
 * Macro which can be used to check the error code. If the code is not CC_OK, it prints the message and returns.
 */
#ifndef CC_RETURN_ON_ERROR
#define CC_RETURN_ON_ERROR(x, log_tag, format, ...)                                                                    \
    do {                                                                                                               \
        cc_master_err_t err_rc_ = (x);                                                                                 \
        if (err_rc_ != CC_OK) {                                                                                        \
            CC_LOGE(log_tag, "%s(%d): " format, __FUNCTION__, __LINE__ __VA_OPT__(, ) __VA_ARGS__);                    \
            return err_rc_;                                                                                            \
        }                                                                                                              \
    } while (0)
#endif

/**
 * Macro which can be used to check the condition. If the condition is not 'true', it prints the message,
 * sets the local variable 'ret' to the supplied 'err_code', and then exits by jumping to 'goto_tag'.
 */
#ifndef CC_GOTO_ON_FALSE
#define CC_GOTO_ON_FALSE(a, err_code, goto_tag, log_tag, format, ...)                                                  \
    do {                                                                                                               \
        if (!(a)) {                                                                                                    \
            CC_LOGE(log_tag, "%s(%d): " format, __FUNCTION__, __LINE__ __VA_OPT__(, ) __VA_ARGS__);                    \
            ret = err_code;                                                                                            \
            goto goto_tag;                                                                                             \
        }                                                                                                              \
    } while (0)
#endif

/**
 * Macro which can be used to check the error code. If the code is not CC_OK, it prints the message,
 * sets the local variable 'ret' to the code, and then exits by jumping to 'goto_tag'.
 */
#ifndef CC_GOTO_ON_ERROR
#define CC_GOTO_ON_ERROR(x, goto_tag, log_tag, format, ...)                                                            \
    do {                                                                                                               \
        (void)log_tag;                                                                                                 \
        cc_master_err_t err_rc_ = (x);                                                                                 \
        if (err_rc_ != CC_OK) {                                                                                        \
            ret = err_rc_;                                                                                             \
            goto goto_tag;                                                                                             \
        }                                                                                                              \
    } while (0)
#endif

//======================================================================================================================
//                                                   FUNCTION PROTOTYPES
//======================================================================================================================

//======================================================================================================================
//                                                   GLOBAL VARIABLES
//======================================================================================================================

//======================================================================================================================
//                                                   PUBLIC FUNCTIONS
//======================================================================================================================

void cc_master_init(cc_master_ctx_t *ctx, cc_master_uart_cb_cfg_t *uart_cb_cfg, void *uart_userdata,
                    cc_master_cb_cfg_t *master_cb_cfg, cc_prop_t *prop_list, size_t prop_list_size, void *prop_userdata)
{
    assert(ctx != NULL);
    assert(master_cb_cfg != NULL);
    assert(master_cb_cfg->node_cnt_update != NULL);
    assert(master_cb_cfg->node_exists_and_must_be_written != NULL);
    assert(master_cb_cfg->node_error_set != NULL);
    assert(uart_cb_cfg != NULL);
    assert(uart_cb_cfg->read != NULL);
    assert(uart_cb_cfg->write != NULL);
    assert(uart_cb_cfg->read_timeout_set != NULL);
    assert(uart_cb_cfg->flush_rx_buff != NULL);
    assert(uart_cb_cfg->wait_tx_done != NULL);
    assert(prop_list != NULL);
    assert(prop_list_size > 0);

    ctx->prop_list      = prop_list;
    ctx->prop_list_size = prop_list_size;
    ctx->prop_userdata  = prop_userdata;
    ctx->uart           = *uart_cb_cfg;
    ctx->uart_userdata  = uart_userdata;
    ctx->master         = *master_cb_cfg;

    uart_cb_cfg->read_timeout_set(uart_userdata, CC_NODE_TIMEOUT_MS);

    ctx->queued.state = CC_MASTER_QUEUE_STATE_UNDEFINED;
}

//----------------------------------------------------------------------------------------------------------------------

cc_master_err_t cc_master_prop_read(cc_master_ctx_t *ctx, cc_prop_id_t property_id)
{
    cc_master_err_t err = CC_MASTER_OK;
    CC_RETURN_ON_FALSE(ctx != NULL, CC_MASTER_ERR_INVALID_ARG, TAG, "ctx is NULL");

    /* Initiate the message. */
    cc_msg_header_t tx_header = {0};
    cc_header_action_set(&tx_header, CC_ACTION_READ);

    if (property_id == CC_MASTER_READ_NODE_ERRORS) {
        /* Prepare to read node errors instead of property. */
        cc_header_read_error_bit_set(&tx_header, true);
        CC_LOGI(TAG, "Reading Node Errors from all Nodes.");

    } else {
        cc_header_property_set(&tx_header, property_id);

        /* Check if the property exists. */
        CC_RETURN_ON_FALSE(property_id < ctx->prop_list_size, CC_MASTER_ERR_NOT_SUPPORTED, TAG,
                           "Property (%d) does not exist", property_id);

        /* Check if the property handler supports writing. (We read from the nodes and write to the master. ) */
        CC_RETURN_ON_FALSE(ctx->prop_list[property_id].handler.set != NULL, CC_MASTER_ERR_NOT_SUPPORTED, TAG,
                           "Property (%d) %s is not readable.", property_id,
                           ctx->prop_list[property_id].attribute.name);

        CC_LOGI(TAG, "Reading Property (%d) %s from all Nodes.", property_id,
                ctx->prop_list[property_id].attribute.name);
    }

    /* Update the header parity.*/
    cc_header_parity_set(&tx_header, true);

    /* Flush uart RX buffer. */
    ctx->uart.flush_rx_buff(ctx->uart_userdata);

    /* Send the header. */
    CC_RETURN_ON_FALSE(ctx->uart.write(ctx->uart_userdata, tx_header.raw, CC_ACTION_HEADER_SIZE) ==
                           CC_ACTION_HEADER_SIZE,
                       CC_MASTER_ERR_FAIL, TAG, "Failed to send header");

    /* Receive the header. */
    cc_msg_header_t rx_header = {0};
    CC_RETURN_ON_FALSE(ctx->uart.read(ctx->uart_userdata, rx_header.raw, CC_ACTION_HEADER_SIZE) ==
                           CC_ACTION_HEADER_SIZE,
                       CC_MASTER_ERR_TIMEOUT, TAG, "Failed to receive header");

    /* Check header integrity. */
    CC_RETURN_ON_FALSE(cc_header_parity_check(rx_header), CC_MASTER_ERR_FAIL, TAG, "Header parity invalid");
    CC_RETURN_ON_FALSE(cc_header_action_get(rx_header) == CC_ACTION_READ, CC_MASTER_ERR_FAIL, TAG,
                       "Header action corrupted");

    if (property_id == CC_MASTER_READ_NODE_ERRORS) {
        CC_RETURN_ON_FALSE(cc_header_read_error_bit_get(rx_header), CC_MASTER_ERR_FAIL, TAG,
                           "Header read error bit not set");
        CC_RETURN_ON_FALSE(cc_header_property_get(rx_header) == 0, CC_MASTER_ERR_FAIL, TAG,
                           "Header property corrupted");
    } else {
        CC_RETURN_ON_FALSE(!cc_header_read_error_bit_get(rx_header), CC_MASTER_ERR_FAIL, TAG,
                           "Header read error bit set");
        CC_RETURN_ON_FALSE(cc_header_property_get(rx_header) == property_id, CC_MASTER_ERR_FAIL, TAG,
                           "Header property corrupted");
    }

    /* Update the node count. */
    uint16_t node_cnt = cc_header_node_cnt_get(rx_header);
    ctx->master.node_cnt_update(ctx->prop_userdata, node_cnt);

    /* Receive the data. */
    for (uint16_t i = 0; i < node_cnt; i++) {
        uint8_t property_data[CC_PAYLOAD_SIZE_MAX] = {0};
        size_t data_size                           = CC_PROPERTY_SIZE_MAX;

        /* Read data until the end of the message. */
        size_t read_cnt = 0;
        do {
            CC_RETURN_ON_FALSE(ctx->uart.read(ctx->uart_userdata, property_data + read_cnt, 1) == 1,
                               CC_MASTER_ERR_TIMEOUT, TAG, "Failed to receive read all data");
            read_cnt++;
        } while (property_data[read_cnt - 1] != 0x00 && read_cnt < CC_PAYLOAD_SIZE_MAX);

        /* Decode the payload. */
        CC_RETURN_ON_FALSE(cc_payload_cobs_decode(property_data, &data_size, property_data, read_cnt),
                           CC_MASTER_ERR_COBS_DEC, TAG, "Failed to decode COBS payload");

        if (data_size == 0) {
            // No data received for this node, skip processing.
            continue;
        }

        /* Verify the checksum. */
        CC_RETURN_ON_FALSE(cc_checksum_calculate(property_data, data_size) == CC_CHECKSUM_OK, CC_MASTER_ERR_CHECKSUM,
                           TAG, "Payload checksum invalid");
        data_size--; /* Remove checksum from size. */

        /* Handle the data. */
        if (property_id == CC_MASTER_READ_NODE_ERRORS) {
            ctx->master.node_error_set(ctx->prop_userdata, i, (cc_node_err_t)property_data[0],
                                       (cc_node_state_t)property_data[1]);
        } else {
            if (!ctx->prop_list[property_id].handler.set(ctx->prop_userdata, i, property_data, &data_size)) {
                err = CC_MASTER_ERR_WRITE_CB;
                CC_LOGE(TAG, "\"set\" callback failed for node %d", i);
            }
        }
    }

    return err;
}

//----------------------------------------------------------------------------------------------------------------------

cc_master_err_t cc_master_prop_write(cc_master_ctx_t *ctx, cc_prop_id_t property_id, uint16_t node_cnt,
                                     bool staged_write, bool broadcast)
{
    CC_RETURN_ON_FALSE(ctx != NULL, CC_MASTER_ERR_INVALID_ARG, TAG, "ctx is NULL");

    /* Check if the property exists. */
    CC_RETURN_ON_FALSE(property_id < ctx->prop_list_size, CC_MASTER_ERR_NOT_SUPPORTED, TAG,
                       "Property (%d) does not exist", property_id);

    /* Check if the property handler supports reading. (We read from the master and write to the nodes. ) */
    CC_RETURN_ON_FALSE(ctx->prop_list[property_id].handler.get != NULL, CC_MASTER_ERR_NOT_SUPPORTED, TAG,
                       "Property (%d) %s is not readable.", property_id, ctx->prop_list[property_id].attribute.name);

    /* Check If the node count is valid. */
    CC_RETURN_ON_FALSE(broadcast || node_cnt > 0, CC_MASTER_ERR_INVALID_ARG, TAG,
                       "Node count must be greater than zero in non-broadcast mode");

    CC_LOGI(TAG, "Writing Property (%d) %s to all Nodes.", property_id, ctx->prop_list[property_id].attribute.name);

    /* Initiate the message. */
    cc_msg_header_t tx_header = {0};
    cc_header_action_set(&tx_header, broadcast ? CC_ACTION_BROADCAST : CC_ACTION_WRITE);
    cc_header_staging_bit_set(&tx_header, staged_write);
    cc_header_property_set(&tx_header, property_id);
    cc_header_node_cnt_set(&tx_header, broadcast ? 0 : node_cnt);
    cc_header_parity_set(&tx_header, true);

    /* Flush uart RX buffer. */
    ctx->uart.flush_rx_buff(ctx->uart_userdata);

    /* Send the header. */
    CC_RETURN_ON_FALSE(ctx->uart.write(ctx->uart_userdata, tx_header.raw, CC_ACTION_HEADER_SIZE) ==
                           CC_ACTION_HEADER_SIZE,
                       CC_MASTER_ERR_FAIL, TAG, "Failed to send header");

    node_cnt = broadcast ? 1 : node_cnt; /* Update node count for broadcast mode */

    size_t property_size                       = 0;
    uint8_t property_data[CC_PAYLOAD_SIZE_MAX] = {0};
    /* Write the data.*/
    for (int16_t i = node_cnt - 1; i >= 0; i--) {
        size_t unencoded_size = 0;

        /* Get the property data. */
        CC_RETURN_ON_FALSE(ctx->prop_list[property_id].handler.get(
                               ctx->prop_userdata, i, property_data + CC_COBS_OVERHEAD_SIZE - 1, &unencoded_size),
                           CC_MASTER_ERR_READ_CB, TAG, "\"get\" callback failed for node %d", i);

        assert(unencoded_size <= CC_PROPERTY_SIZE_MAX);

        /* Append checksum. */
        property_data[CC_COBS_OVERHEAD_SIZE - 1 + unencoded_size] =
            cc_checksum_calculate(property_data + CC_COBS_OVERHEAD_SIZE - 1, unencoded_size);

        /* Encode the property data. */
        property_size = sizeof(property_data);
        CC_RETURN_ON_FALSE(cc_payload_cobs_encode(property_data, &property_size,
                                                  property_data + CC_COBS_OVERHEAD_SIZE - 1, unencoded_size + 1),
                           CC_MASTER_ERR_COBS_ENC, TAG, "Failed to encode COBS payload");

        /* Send the property data. */
        CC_RETURN_ON_FALSE(ctx->uart.write(ctx->uart_userdata, property_data, property_size) == property_size,
                           CC_MASTER_ERR_FAIL, TAG, "Failed to send property data");
    }

    /* Receive the header. */
    cc_msg_header_t rx_header = {0};
    CC_RETURN_ON_FALSE(ctx->uart.read(ctx->uart_userdata, rx_header.raw, CC_ACTION_HEADER_SIZE) ==
                           CC_ACTION_HEADER_SIZE,
                       CC_MASTER_ERR_TIMEOUT, TAG, "Failed to receive header");

    /* Check header integrity. */
    CC_RETURN_ON_FALSE(cc_header_parity_check(rx_header), CC_MASTER_ERR_FAIL, TAG, "Header parity invalid");
    CC_RETURN_ON_FALSE(cc_header_action_get(rx_header) == (broadcast ? CC_ACTION_BROADCAST : CC_ACTION_WRITE),
                       CC_MASTER_ERR_FAIL, TAG, "Header action corrupted");
    CC_RETURN_ON_FALSE(cc_header_property_get(rx_header) == property_id, CC_MASTER_ERR_FAIL, TAG,
                       "Header property corrupted");

    /* Update the node count. */
    uint16_t node_cnt_rx = cc_header_node_cnt_get(rx_header);

    /* When we are not in broadcast mode, we expect the node count to be zero, If it is more than zero, the number of
     * nodes has dropped. It can't be less than zero because that would crash the node. When not in broadcast mode, the
     * node count shall be the number of nodes in the system. */
    if (!broadcast) {
        node_cnt -= node_cnt_rx;
    } else {
        node_cnt = node_cnt_rx;
    }

    ctx->master.node_cnt_update(ctx->prop_userdata, node_cnt);

    if (broadcast) {
        /* Receive the property data */
        uint8_t property_data_rx[CC_PROPERTY_SIZE_MAX] = {0};
        CC_RETURN_ON_FALSE(ctx->uart.read(ctx->uart_userdata, property_data_rx, property_size) == property_size,
                           CC_MASTER_ERR_TIMEOUT, TAG, "Failed to receive property data");

        /* Compare received property with transmitted property data. */
        CC_RETURN_ON_FALSE(memcmp(property_data, property_data_rx, property_size) == 0,
                           CC_MASTER_ERR_BROADCAST_CORRUPTED, TAG, "Broadcast data corrupted");
    }

    return CC_MASTER_OK;
}

//----------------------------------------------------------------------------------------------------------------------

cc_master_err_t cc_master_sync_ack(cc_master_ctx_t *ctx, cc_sync_error_code_t *node_errors_present)
{
    CC_RETURN_ON_FALSE(ctx != NULL, CC_MASTER_ERR_INVALID_ARG, TAG, "ctx is NULL");
    CC_RETURN_ON_FALSE(node_errors_present != NULL, CC_MASTER_ERR_INVALID_ARG, TAG, "node_errors_present is NULL");

    cc_msg_header_t tx_header = {0};
    cc_header_action_set(&tx_header, CC_ACTION_SYNC);
    cc_header_sync_type_set(&tx_header, CC_SYNC_ACK);

    /* Flush uart RX buffer. */
    ctx->uart.flush_rx_buff(ctx->uart_userdata);

    /* Send the header. */
    ctx->uart.wait_tx_done(ctx->uart_userdata);
    CC_RETURN_ON_FALSE(ctx->uart.write(ctx->uart_userdata, tx_header.raw, CC_SYNC_HEADER_SIZE) == CC_SYNC_HEADER_SIZE,
                       CC_MASTER_ERR_FAIL, TAG, "Failed to send SYNC header");

    /* Receive the header. */
    cc_msg_header_t rx_header = {0};
    CC_RETURN_ON_FALSE(ctx->uart.read(ctx->uart_userdata, rx_header.raw, CC_SYNC_HEADER_SIZE) == CC_SYNC_HEADER_SIZE,
                       CC_MASTER_ERR_TIMEOUT, TAG, "Failed to receive SYNC header");

    /* Check for header corruption. */
    CC_RETURN_ON_FALSE(cc_header_action_get(rx_header) == CC_ACTION_SYNC, CC_MASTER_ERR_FAIL, TAG,
                       "Header action corrupted");
    CC_RETURN_ON_FALSE(cc_header_sync_type_get(rx_header) == CC_SYNC_ACK, CC_MASTER_ERR_FAIL, TAG,
                       "Header sync type corrupted");

    /* Check header error bits. */
    *node_errors_present = cc_header_sync_error_get(rx_header);

    return CC_MASTER_OK;
}

//----------------------------------------------------------------------------------------------------------------------

cc_master_err_t cc_master_prop_node_commit_prop(cc_master_ctx_t *ctx)
{
    CC_RETURN_ON_FALSE(ctx != NULL, CC_MASTER_ERR_INVALID_ARG, TAG, "ctx is NULL");

    cc_msg_header_t tx_header = {0};
    cc_header_action_set(&tx_header, CC_ACTION_SYNC);
    cc_header_sync_type_set(&tx_header, CC_SYNC_COMMIT);

    /* Flush uart RX buffer. */
    ctx->uart.flush_rx_buff(ctx->uart_userdata);

    /* Send the header. */
    ctx->uart.wait_tx_done(ctx->uart_userdata);
    CC_RETURN_ON_FALSE(ctx->uart.write(ctx->uart_userdata, tx_header.raw, CC_SYNC_HEADER_SIZE) == CC_SYNC_HEADER_SIZE,
                       CC_MASTER_ERR_FAIL, TAG, "Failed to send SYNC header");

    /* Receive the header. */
    cc_msg_header_t rx_header = {0};
    CC_RETURN_ON_FALSE(ctx->uart.read(ctx->uart_userdata, rx_header.raw, CC_SYNC_HEADER_SIZE) == CC_SYNC_HEADER_SIZE,
                       CC_MASTER_ERR_TIMEOUT, TAG, "Failed to receive SYNC header");

    /* Check for header corruption. */
    CC_RETURN_ON_FALSE(cc_header_action_get(rx_header) == CC_ACTION_SYNC, CC_MASTER_ERR_FAIL, TAG,
                       "Header action corrupted");
    CC_RETURN_ON_FALSE(cc_header_sync_type_get(rx_header) == CC_SYNC_COMMIT, CC_MASTER_ERR_FAIL, TAG,
                       "Header sync type corrupted");
    /* Error bits should not be set in commit action. */
    CC_RETURN_ON_FALSE(cc_header_sync_error_get(rx_header) == 0, CC_MASTER_ERR_FAIL, TAG,
                       "Header error bits corrupted");

    return CC_MASTER_OK;
}

//----------------------------------------------------------------------------------------------------------------------

cc_master_err_t cc_master_queue_prop_read(cc_master_ctx_t *ctx, cc_prop_id_t property_id)
{
    CC_RETURN_ON_FALSE(ctx != NULL, CC_MASTER_ERR_INVALID_ARG, TAG, "ctx is NULL");

    /* Check if another action is already queued. */
    CC_RETURN_ON_FALSE(ctx->queued.state == CC_MASTER_QUEUE_STATE_UNDEFINED, CC_MASTER_ERR_QUEUE_FULL, TAG,
                       "Another action is already queued");

    /* Queue the read action. */
    ctx->queued.state       = CC_MASTER_QUEUE_STATE_READ;
    ctx->queued.prop_id     = property_id;
    ctx->queued.attempt_cnt = 0;

    return CC_MASTER_OK;
}

//----------------------------------------------------------------------------------------------------------------------

cc_master_err_t cc_master_queue_prop_write(cc_master_ctx_t *ctx, cc_prop_id_t property_id, uint16_t node_cnt,
                                           bool broadcast)
{
    CC_RETURN_ON_FALSE(ctx != NULL, CC_MASTER_ERR_INVALID_ARG, TAG, "ctx is NULL");

    /* Check if another action is already queued. */
    CC_RETURN_ON_FALSE(ctx->queued.state == CC_MASTER_QUEUE_STATE_UNDEFINED, CC_MASTER_ERR_QUEUE_FULL, TAG,
                       "Another action is already queued");

    /* Queue the write action. */
    ctx->queued.state       = CC_MASTER_QUEUE_STATE_WRITE;
    ctx->queued.prop_id     = property_id;
    ctx->queued.node_cnt    = node_cnt;
    ctx->queued.broadcast   = broadcast;
    ctx->queued.attempt_cnt = 0;

    return CC_MASTER_OK;
}

//----------------------------------------------------------------------------------------------------------------------

cc_master_err_t cc_master_communication_handler(cc_master_ctx_t *ctx, uint32_t *next_tick_ms)
{
    CC_RETURN_ON_FALSE(ctx != NULL, CC_MASTER_ERR_INVALID_ARG, TAG, "ctx is NULL");
    CC_RETURN_ON_FALSE(next_tick_ms != NULL, CC_MASTER_ERR_INVALID_ARG, TAG, "next_tick_ms is NULL");

    cc_master_err_t err                      = CC_MASTER_OK;
    cc_sync_error_code_t node_errors_present = CC_SYNC_ERR_NONE;

    switch (ctx->queued.state) {
        case CC_MASTER_QUEUE_STATE_READ:
            err           = cc_master_prop_read(ctx, ctx->queued.prop_id);
            *next_tick_ms = (err == CC_MASTER_OK) ? 0 : CC_NODE_RECOVERY_DELAY_MS;
            break;

        case CC_MASTER_QUEUE_STATE_WRITE:
            err = cc_master_prop_write(ctx, ctx->queued.prop_id, ctx->queued.node_cnt, false, ctx->queued.broadcast);
            *next_tick_ms = (err == CC_MASTER_OK) ? 0 : CC_NODE_RECOVERY_DELAY_MS;
            if (err == CC_MASTER_OK) {
                ctx->queued.state = CC_MASTER_QUEUE_STATE_SYNC_ACK;
                err = cc_master_communication_handler(ctx, next_tick_ms); /* Immediately proceed to SYNC ACK */
            }
            break;

        case CC_MASTER_QUEUE_STATE_SYNC_ACK:
            err           = cc_master_sync_ack(ctx, &node_errors_present);
            *next_tick_ms = (err == CC_MASTER_OK) ? 0 : CC_NODE_RECOVERY_DELAY_MS;
            if (err == CC_MASTER_OK && node_errors_present != CC_SYNC_ERR_NONE) {
                ctx->queued.state            = CC_MASTER_QUEUE_STATE_READ;
                ctx->queued.original_prop_id = ctx->queued.prop_id; /* Store for later. */
                ctx->queued.prop_id          = CC_MASTER_READ_NODE_ERRORS;
                err = cc_master_communication_handler(ctx, next_tick_ms); /* Immediately proceed to READ NODE ERRORS */
                *next_tick_ms = (err == CC_MASTER_OK) ? 0 : CC_NODE_RECOVERY_DELAY_MS;
                if (err == CC_MASTER_OK) {
                    /* Restore original property ID for next attempt. */
                    ctx->queued.prop_id = ctx->queued.original_prop_id;
                    ctx->queued.state   = CC_MASTER_QUEUE_STATE_WRITE;
                    err                 = CC_MASTER_ERR_FAIL; /* Indicate that an error was present. */
                }
            }
            break;

        case CC_MASTER_QUEUE_STATE_UNDEFINED:
            err           = CC_MASTER_ERR_QUEUE_EMPTY;
            *next_tick_ms = 0;
            break;

        default:
            err           = CC_MASTER_ERR_INVALID_ARG;
            *next_tick_ms = 0;
            break;
    }

    if (err == CC_MASTER_OK) {
        /* Clear the queued action on success or timeout. */
        ctx->queued.state = CC_MASTER_QUEUE_STATE_UNDEFINED;
    }

    return err;
}