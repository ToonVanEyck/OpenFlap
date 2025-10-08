#include "chain_comm_node.h"

#include <string.h>

#define CTX_PROP ctx->property_list[ctx->header.property - 1]

#ifndef CHAIN_COMM_DEBUG
#define CHAIN_COMM_DEBUG true
#endif

/*
 * Set to true to enable UART RTT trace this will slow down the execution and might cause issues with the chain
 * communication.
 */
#define TRACE_CHAIN_COMM_UART false

#ifndef TAG
#define TAG "chain_comm_node"
#endif

#if !defined(CC_LOGE) || !defined(CC_LOGW) || !defined(CC_LOGI) || !defined(CC_LOGD)
#include <stdio.h>
#define CC_LOGE(tag, format, ...) printf("E: [%s]: " format "\n", tag, ##__VA_ARGS__)
#define CC_LOGW(tag, format, ...) printf("W: [%s]: " format "\n", tag, ##__VA_ARGS__)
#define CC_LOGI(tag, format, ...) printf("I: [%s]: " format "\n", tag, ##__VA_ARGS__)
#define CC_LOGD(tag, format, ...) printf("D: [%s]: " format "\n", tag, ##__VA_ARGS__)
#endif

void cc_node_state_change(cc_node_ctx_t *ctx, cc_node_state_t state);
void cc_node_exec(cc_node_ctx_t *ctx);

/* Chain comm timer functions. */
void cc_node_timer_start(cc_node_ctx_t *ctx);
bool cc_node_timer_elapsed(cc_node_ctx_t *ctx);

/* Chain comm FSM state implementations. */
// void cc_node_state_rxHeader(cc_node_ctx_t *ctx);
// void cc_node_state_rxSize(cc_node_ctx_t *ctx);
// void cc_node_state_readAll_rxCnt(cc_node_ctx_t *ctx);
// void cc_node_state_readAll_rxData(cc_node_ctx_t *ctx);
// void cc_node_state_readAll_txSize(cc_node_ctx_t *ctx);
// void cc_node_state_readAll_txData(cc_node_ctx_t *ctx);
// void cc_node_state_writeAll_rxData(cc_node_ctx_t *ctx);
// void cc_node_state_writeAll_rxAck(cc_node_ctx_t *ctx);
// void cc_node_state_writeSeq_rxData(cc_node_ctx_t *ctx);

/* Chain comm UART functions. */
static size_t cc_node_uart_write(cc_node_ctx_t *ctx, const void *buf, size_t length, uint8_t *checksum);
static size_t cc_node_uart_read(cc_node_ctx_t *ctx, void *buf, size_t length, uint8_t *checksum);

void cc_node_init(cc_node_ctx_t *ctx, const cc_node_uart_cb_cfg_t *uart_cb, void *uart_userdata,
                  cc_prop_t *property_list, size_t property_list_size, void *cc_userdata)
{
    ctx->property_list      = property_list;
    ctx->property_list_size = property_list_size;
    ctx->cc_userdata        = cc_userdata;
    ctx->uart               = *uart_cb;
    ctx->uart_userdata      = uart_userdata;

    ctx->state = rxHeader;

    ctx->timeout_tick_cnt = 0;
    ctx->last_tick_ms     = 0;

    // ctx->header.raw = 0;
    // ctx->data_cnt   = 0;
    // ctx->index      = 0;
    // memset(ctx->property_data, 0, sizeof(ctx->property_data));
    // ctx->msg_rc                 = CC_RC_SUCCESS;
    // ctx->property_size          = 0;
    // ctx->checksum_rx_calc       = 0;
    // ctx->checksum_tx_calc       = 0;
    // ctx->writeSeq_packet_cnt    = 0;
    // ctx->writeSeq_property_size = 0;
    // ctx->writeSeq_header.raw    = 0;
    // ctx->writeSeq_packet_cnt    = 0;
}

bool cc_node_tick(cc_node_ctx_t *ctx, uint32_t tick_ms)
{
    ctx->last_tick_ms = tick_ms;
    switch (ctx->state) {
        // case rxHeader:
        //     cc_node_state_rxHeader(ctx);
        //     break;
        // case rxSize:
        //     cc_node_state_rxSize(ctx);
        //     break;
        // case readAll_rxCnt:
        //     cc_node_state_readAll_rxCnt(ctx);
        //     break;
        // case readAll_rxData:
        //     cc_node_state_readAll_rxData(ctx);
        //     break;
        // case readAll_txSize:
        //     cc_node_state_readAll_txSize(ctx);
        //     break;
        // case readAll_txData:
        //     cc_node_state_readAll_txData(ctx);
        //     break;
        // case writeAll_rxData:
        //     cc_node_state_writeAll_rxData(ctx);
        //     break;
        // case writeAll_rxAck:
        //     cc_node_state_writeAll_rxAck(ctx);
        //     break;
        // case writeSeq_rxData:
        //     cc_node_state_writeSeq_rxData(ctx);
        //     break;
        default:
            CC_LOGD(TAG, "Invalid state");
            break;
    }
    return true;
}

void cc_node_timer_start(cc_node_ctx_t *ctx)
{
    ctx->timeout_tick_cnt = ctx->last_tick_ms + CHAIN_COMM_TIMEOUT_MS;
}

bool cc_node_timer_elapsed(cc_node_ctx_t *ctx)
{
    return ctx->last_tick_ms > ctx->timeout_tick_cnt;
}

bool cc_node_is_busy(cc_node_ctx_t *ctx)
{
    return (ctx->state != rxHeader) || ctx->uart.is_busy(ctx->uart_userdata);
}

// /**
//  * \brief Changes the state of the chain communication FSM.
//  *
//  * This function changes the state of the chain communication FSM to the provided state.
//  * It also resets the rx and tx counters of the context.
//  *
//  * \param[inout] ctx Pointer to the cc_node_ctx_t structure containing the context information.
//  * \param[in] state The new state of the chain communication FSM.
//  */
// void cc_node_state_change(cc_node_ctx_t *ctx, cc_node_state_t state)
// {
//     static const char *state_names[] = {CHAIN_COMM_STATE(GENERATE_STATE_NAME)};
//     CC_LOGD(TAG, "State: %s -> %s", state_names[ctx->state], state_names[state]);
//     if (state != rxHeader) {
//         cc_node_timer_start(ctx);
//     }
//     ctx->state    = state;
//     ctx->data_cnt = 0;
// }

// /**
//  * \brief Executes the chain communication based on the provided context.
//  *
//  * This function handles the execution of chain communication based on the provided context.
//  * It checks the action field of the context header and performs the corresponding action.
//  *
//  * \param[in] ctx Pointer to the cc_node_ctx_t structure containing the context information.
//  */
// void cc_node_exec(cc_node_ctx_t *ctx)
// {
//     if (ctx->header.property == 0 || ctx->header.property > ctx->property_list_size) {
//         return;
//     }
//     if (ctx->header.action == property_readAll) {
//         if (CTX_PROP.handler.get) {
//             ctx->property_size = CTX_PROP.attribute.read_size.static_size;
//             CTX_PROP.handler.get(0, ctx->property_data, &ctx->property_size, ctx->cc_userdata);
//             CC_LOGD(TAG, "Read %s property", CTX_PROP.attribute.name);
//         } else {
//             CC_LOGD(TAG, "Property (%d) not supported", ctx->header.property);
//         }
//     } else if (ctx->header.action == property_writeAll || ctx->header.action == property_writeSequential) {
//         if (CTX_PROP.handler.set) {
//             CTX_PROP.handler.set(0, ctx->property_data, &ctx->property_size, ctx->cc_userdata);
//             CC_LOGD(TAG, "Write %s property", CTX_PROP.attribute.name);
//         } else {
//             CC_LOGD(TAG, "Property (%d) not supported", ctx->header.property);
//         }
//     }
// }

// /**
//  * \brief Handles the rxHeader state of the chain communication FSM.
//  *
//  * This state receives a header byte and changes the state based on the action field of the header.
//  *
//  * \param[inout] ctx Pointer to the cc_node_ctx_t structure containing the context information.
//  */
// void cc_node_state_rxHeader(cc_node_ctx_t *ctx)
// {
//     uint8_t data;
//     ctx->property_size    = 0;
//     ctx->checksum_rx_calc = 0;
//     ctx->checksum_tx_calc = 0;
//     if (ctx->uart.cnt_writable(ctx->uart_userdata) && cc_node_uart_read(ctx, &data, 1, &ctx->checksum_rx_calc)) {
//         ctx->header.raw = data;
//         if (ctx->header.property > ctx->property_list_size) {
//             CC_LOGW(TAG, "Invalid property ID: %d\n", ctx->header.property);
//             cc_node_state_change(ctx, rxHeader);
//         }

//         /* Switch to next state. */
//         switch (ctx->header.action) {
//             case property_readAll:
//                 cc_node_uart_write(ctx, &data, 1, &ctx->checksum_tx_calc);
//                 cc_node_state_change(ctx, readAll_rxCnt);
//                 break;
//             case property_writeAll:
//                 cc_node_uart_write(ctx, &data, 1, NULL);
//                 cc_node_state_change(ctx, rxSize);
//                 break;
//             case property_writeSequential:
//                 if (++ctx->writeSeq_packet_cnt == 1) {
//                     ctx->writeSeq_header.raw = ctx->header.raw;
//                 } else {
//                     cc_node_uart_write(ctx, &data, 1, NULL);
//                 }
//                 cc_node_state_change(ctx, ctx->header.property != 0 ? rxSize : rxHeader);
//                 break;
//             case returnCode:
//                 data |= ctx->msg_rc;
//                 cc_node_uart_write(ctx, &data, 1, NULL);
//                 if (ctx->writeSeq_packet_cnt > 0 /* && ctx->writeSeq_header.property > 0*/) {
//                     if (ctx->msg_rc == CC_RC_SUCCESS) {
//                         /* Restore header and data size before executing. */
//                         ctx->property_size = ctx->writeSeq_property_size;
//                         ctx->header.raw    = ctx->writeSeq_header.raw;
//                         cc_node_exec(ctx);
//                     }
//                 }
//                 ctx->writeSeq_packet_cnt = 0;
//                 ctx->msg_rc              = CC_RC_SUCCESS;
//                 cc_node_state_change(ctx, rxHeader);
//                 break;
//             default:
//                 cc_node_state_change(ctx, rxHeader);
//                 break;
//         }
//     }
// }

// /**
//  * \brief Handles the rxSize state of the chain communication FSM.
//  *
//  * This state set initiates the property size in the context, it either sets the property size based on known static
//  * property sizes, or it receives the property size in case of a dynamic property size.
//  *
//  * \param[inout] ctx Pointer to the cc_node_ctx_t structure containing the context information.
//  */
// void cc_node_state_rxSize(cc_node_ctx_t *ctx)
// {
//     const cc_prop_attr_t *property = &CTX_PROP.attribute;
//     const cc_prop_size_t *property_size =
//         ctx->header.action == property_readAll ? &property->read_size : &property->write_size;

//     uint8_t *cs = (ctx->header.action == property_readAll) ? NULL : &ctx->checksum_tx_calc;

//     /* Get the dynamic size bytes and retransmit the for readAll and writeAll properties. */
//     if (property_size->is_dynamic) {
//         while (ctx->uart.cnt_writable(ctx->uart_userdata) && ctx->data_cnt < 2 &&
//                cc_node_uart_read(ctx, &((uint8_t *)&ctx->property_size)[ctx->data_cnt], 1, &ctx->checksum_rx_calc)) {
//             if (ctx->header.action == property_writeAll || ctx->header.action == property_readAll ||
//                 (ctx->header.action == property_writeSequential && ctx->writeSeq_packet_cnt > 1)) { /* TODO check
//                 !=1*/ cc_node_uart_write(ctx, &((uint8_t *)&ctx->property_size)[ctx->data_cnt], 1, cs);
//             }
//             ctx->data_cnt++;
//         };
//     } else {
//         ctx->property_size = property_size->static_size;
//     }

//     /* Switch mode. */
//     if (ctx->data_cnt == 2 || !property_size->is_dynamic) {
//         // debug_io_log_debug("Property size: %d\n", ctx->property_size);
//         if (ctx->header.action == property_writeAll) {
//             cc_node_state_change(ctx, writeAll_rxData);
//         } else if (ctx->header.action == property_writeSequential) {
//             cc_node_state_change(ctx, writeSeq_rxData);
//         } else if (ctx->header.action == property_readAll) {
//             cc_node_state_change(ctx, readAll_rxData);
//         }
//     } else if (cc_node_timer_elapsed(ctx)) {
//         CC_LOGW(TAG, "Timeout during %s\n", __FUNCTION__);
//         cc_node_state_change(ctx, rxHeader);
//     }
// }

// /**
//  * \brief Handles the readAll_rxCnt state of the chain communication FSM.
//  *
//  * This state receives 2 count bytes which the modules increment, this allows each successive module to know how
//  * many modules are in the chain before this one. As soon as the second byte is received, the module will execute
//  * property read callback.
//  *
//  * \param[inout] ctx Pointer to the cc_node_ctx_t structure containing the context information.
//  */
// void cc_node_state_readAll_rxCnt(cc_node_ctx_t *ctx)
// {
//     uint8_t data;
//     if (ctx->uart.cnt_writable(ctx->uart_userdata) && cc_node_uart_read(ctx, &data, 1, NULL)) {
//         ctx->data_cnt++;
//         if (ctx->data_cnt == 1) {
//             ctx->index = ((uint16_t)data) + 1;
//             data       = (ctx->index >> 0) & 0xff;
//         } else {
//             ctx->index += (data << 8);
//             data = (ctx->index >> 8) & 0xff;
//             ctx->checksum_tx_calc += (uint8_t)((ctx->index) >> 8) + (uint8_t)(ctx->index);
//             cc_node_exec(ctx);
//             if (--ctx->index) {
//                 cc_node_state_change(ctx, rxSize);
//             } else {
//                 if (CTX_PROP.attribute.read_size.is_dynamic) {
//                     cc_node_state_change(ctx, readAll_txSize);
//                 } else {
//                     cc_node_state_change(ctx, readAll_txData);
//                 }
//             }
//         }
//         cc_node_uart_write(ctx, &data, 1, NULL);
//     } else if (cc_node_timer_elapsed(ctx)) {
//         CC_LOGW(TAG, "Timeout during %s", __FUNCTION__);
//         cc_node_state_change(ctx, rxHeader);
//     }
// }

// /**
//  * \brief Handles the readAll_rxData state of the chain communication FSM.
//  *
//  * This state receives the data bytes written by the readAll operation of the previous module. These bytes are
//  * forwarded to the next modules. Once all data of the previous modules has been forwarded, the module will change
//  * the state to readAll_txData.
//  *
//  * \param[inout] ctx Pointer to the cc_node_ctx_t structure containing the context information.
//  */
// void cc_node_state_readAll_rxData(cc_node_ctx_t *ctx)
// {
//     uint8_t data;
//     /* Add 1 to size to account for checksum. */
//     while (ctx->uart.cnt_writable(ctx->uart_userdata) && ctx->data_cnt < ctx->property_size + 1 &&
//            cc_node_uart_read(ctx, &data, 1, NULL)) {
//         ctx->data_cnt++;
//         cc_node_uart_write(ctx, &data, 1, NULL);
//     }
//     if (ctx->data_cnt == ctx->property_size + 1) {
//         if (--ctx->index) {
//             cc_node_state_change(ctx, rxSize);
//         } else {
//             if (CTX_PROP.attribute.read_size.is_dynamic) {
//                 cc_node_state_change(ctx, readAll_txSize);
//             } else {
//                 cc_node_state_change(ctx, readAll_txData);
//             }
//         }
//     } else if (cc_node_timer_elapsed(ctx)) {
//         CC_LOGW(TAG, "Timeout during %s\n", __FUNCTION__);
//         cc_node_state_change(ctx, rxHeader);
//     }
// }

// /**
//  * \brief Handles the readAll_txSize state of the chain communication FSM.
//  *
//  * This state sends the property size to the next module in the chain. Once the size has been sent, the state will
//  * change to the txData state.
//  *
//  * \param[inout] ctx Pointer to the cc_node_ctx_t structure containing the context information.
//  */
// void cc_node_state_readAll_txSize(cc_node_ctx_t *ctx)
// {
//     if (ctx->data_cnt == 0 && ctx->uart.cnt_writable(ctx->uart_userdata) >= 2) {
//         /* Send dynamic size. */
//         cc_node_uart_write(ctx, (uint8_t *)&ctx->property_size, 2, &ctx->checksum_tx_calc);
//         cc_node_state_change(ctx, readAll_txData);
//     } else if (cc_node_timer_elapsed(ctx)) {
//         CC_LOGW(TAG, "Timeout during %s\n", __FUNCTION__);
//         cc_node_state_change(ctx, rxHeader);
//     }
// }

// /**
//  * \brief Handles the readAll_txData state of the chain communication FSM.
//  *
//  * This state sends the data bytes of the readAll operation to the next module. Once all data has been sent, the
//  * state will change to rxHeader.
//  *
//  * \param[inout] ctx Pointer to the cc_node_ctx_t structure containing the context information.
//  */
// void cc_node_state_readAll_txData(cc_node_ctx_t *ctx)
// {
//     /* Send data. */
//     uint8_t data_cnt = cc_node_uart_write(ctx, &ctx->property_data[ctx->data_cnt], ctx->property_size -
//     ctx->data_cnt,
//                                           &ctx->checksum_tx_calc);
//     ctx->data_cnt += data_cnt;
//     if (ctx->data_cnt == ctx->property_size && ctx->uart.cnt_writable(ctx->uart_userdata)) {
//         cc_node_uart_write(ctx, &ctx->checksum_tx_calc, 1, NULL); /* Transmit checksum. */
//         cc_node_state_change(ctx, rxHeader);
//     } else if (cc_node_timer_elapsed(ctx)) {
//         CC_LOGW(TAG, "Timeout during %s\n", __FUNCTION__);
//         cc_node_state_change(ctx, rxHeader);
//     }
// }

// /**
//  * \brief Handles the writeAll_rxData state of the chain communication FSM.
//  *
//  * This state receives the data bytes to be written by the writeAll operation. All data is received is also passed
//  * to the next module. Once all data has been forwarded to the next module, the module will execute the writeAll
//  * command for the property and change the state to writeAll_rxAck.
//  *
//  * \param[inout] ctx Pointer to the cc_node_ctx_t structure containing the context information.
//  */
// void cc_node_state_writeAll_rxData(cc_node_ctx_t *ctx)
// {
//     uint8_t cs = 0;
//     while (ctx->uart.cnt_writable(ctx->uart_userdata) && ctx->data_cnt < ctx->property_size &&
//            cc_node_uart_read(ctx, &ctx->property_data[ctx->data_cnt], 1, &ctx->checksum_rx_calc)) {
//         if (!cc_node_uart_write(ctx, &ctx->property_data[ctx->data_cnt], 1, NULL)) {
//             // debug_io_log_error("Error writing data to uart\n");
//         };
//         ctx->data_cnt++;
//     }
//     if (ctx->data_cnt == ctx->property_size && ctx->uart.cnt_writable(ctx->uart_userdata) &&
//         cc_node_uart_read(ctx, &cs, 1, NULL)) { /* RX checksum. */
//         cc_node_uart_write(ctx, &cs, 1, NULL);  /* TX checksum. */
//         ctx->data_cnt++;
//         if (cs != ctx->checksum_rx_calc) {
//             ctx->msg_rc = CC_RC_ERROR_CHECKSUM;
//             cc_node_state_change(ctx, writeAll_rxAck);
//         }
//     } else if (ctx->data_cnt == ctx->property_size + 1 &&
//                ctx->uart.tx_buff_empty(ctx->uart_userdata)) { // All bytes have been transmitted.
//         cc_node_exec(ctx);
//         ctx->msg_rc = CC_RC_SUCCESS;
//         cc_node_state_change(ctx, writeAll_rxAck);
//     } else if (cc_node_timer_elapsed(ctx)) {
//         CC_LOGW(TAG, "Timeout during %s\n", __FUNCTION__);
//         cc_node_state_change(ctx, rxHeader);
//     }
// }
// /**
//  * \brief Handles the writeAll_rxAck state of the chain communication FSM.
//  *
//  * This state waits for an acknowledgement from the previous module. Once the acknowledgement has been received, the
//  * state will change to rxHeader.
//  *
//  * \param[inout] ctx Pointer to the cc_node_ctx_t structure containing the context information.
//  */
// void cc_node_state_writeAll_rxAck(cc_node_ctx_t *ctx)
// {
//     uint8_t return_code = 0;
//     if (ctx->uart.cnt_writable(ctx->uart_userdata) && cc_node_uart_read(ctx, &return_code, 1, NULL)) {
//         return_code |= ctx->msg_rc;
//         cc_node_uart_write(ctx, &return_code, 1, NULL);
//         cc_node_state_change(ctx, rxHeader);
//     } else if (cc_node_timer_elapsed(ctx)) {
//         CC_LOGW(TAG, "Timeout during %s\n", __FUNCTION__);
//         cc_node_state_change(ctx, rxHeader);
//     }
// }

// /**
//  * \brief Handles the writeSeq_rxData state of the chain communication FSM.
//  *
//  * This state receives the data bytes to be written by the writeSequential operation. Initially the data is received
//  and
//  * stored for later use without retransmitting it. Once a full packet has been received, subsequent packets are
//  * forwarded to the next module. Once all data has been forwarded to the next module.
//  *
//  * \param[inout] ctx Pointer to the cc_node_ctx_t structure containing the context information.
//  */
// void cc_node_state_writeSeq_rxData(cc_node_ctx_t *ctx)
// {
//     uint8_t cs = 0;

//     /* Read data to dummy registers if we are just passing data through. */
//     uint8_t dummy       = 0;
//     uint8_t *msg_rc_ptr = &dummy;
//     uint8_t *data_ptr   = &dummy;

//     if (ctx->writeSeq_packet_cnt == 1) {
//         ctx->writeSeq_property_size = ctx->property_size;
//         msg_rc_ptr                  = (uint8_t *)&ctx->msg_rc;
//         data_ptr                    = &ctx->property_data[ctx->data_cnt];
//     }

//     if (ctx->data_cnt < ctx->property_size && cc_node_uart_read(ctx, data_ptr, 1, &ctx->checksum_rx_calc)) {
//         if (ctx->writeSeq_packet_cnt > 1) {
//             cc_node_uart_write(ctx, data_ptr, 1, NULL);
//         }
//         ctx->data_cnt++;
//     } else if (ctx->data_cnt == ctx->property_size && cc_node_uart_read(ctx, &cs, 1, NULL)) {
//         if (ctx->writeSeq_packet_cnt > 1) {
//             cc_node_uart_write(ctx, &cs, 1, NULL);
//         }
//         if (cs == ctx->checksum_rx_calc) {
//             *msg_rc_ptr = CC_RC_SUCCESS;
//         } else {
//             *msg_rc_ptr = CC_RC_ERROR_CHECKSUM;
//         }
//         cc_node_state_change(ctx, rxHeader);
//     } else if (cc_node_timer_elapsed(ctx)) {
//         CC_LOGW(TAG, "Timeout during %s\n", __FUNCTION__);
//         cc_node_state_change(ctx, rxHeader);
//         ctx->writeSeq_packet_cnt = 0;
//     }
// }

//---------------------------------------------------------------------------------------------------------------------

static size_t cc_node_uart_write(cc_node_ctx_t *ctx, const void *buf, size_t length, uint8_t *checksum)
{
    size_t s = ctx->uart.write(ctx->uart_userdata, buf, length);
    // for (size_t i = 0; i < s; i++) {
    //     CC_LOGD(TAG, "W: 0x%02X", ((const uint8_t *)buf)[i]);
    // }
    for (size_t i = 0; checksum != NULL && i < s; i++) {

        *checksum += ((const uint8_t *)buf)[i];
    }
    return s;
}

//---------------------------------------------------------------------------------------------------------------------

static size_t cc_node_uart_read(cc_node_ctx_t *ctx, void *buf, size_t length, uint8_t *checksum)
{
    size_t s = ctx->uart.read(ctx->uart_userdata, buf, length);
    // for (size_t i = 0; i < s; i++) {
    //     CC_LOGD(TAG, "R: 0x%02X", ((const uint8_t *)buf)[i]);
    // }
    for (size_t i = 0; checksum != NULL && i < s; i++) {
        *checksum += ((const uint8_t *)buf)[i];
    }
    return s;
}

//---------------------------------------------------------------------------------------------------------------------
