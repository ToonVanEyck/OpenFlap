#include "chain_comm_master.h"

#include <string.h>

#define CTX_PROP ctx->property_list[property_id - 1]

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
//----------------------------------------------------------------------------------------------------------------------

void cc_master_init(cc_master_ctx_t *ctx, cc_master_uart_cb_cfg_t *uart_cb_cfg, void *uart_userdata,
                    cc_master_cb_cfg_t *master_cb_cfg, cc_prop_t *property_list, size_t property_list_size,
                    void *cc_userdata)
{
    ctx->property_list      = property_list;
    ctx->property_list_size = property_list_size;
    ctx->cc_userdata        = cc_userdata;
    ctx->uart               = *uart_cb_cfg;
    ctx->uart_userdata      = uart_userdata;
    ctx->master             = *master_cb_cfg;
}

//----------------------------------------------------------------------------------------------------------------------

cc_master_err_t cc_master_prop_read_all(cc_master_ctx_t *ctx, cc_prop_id_t property_id)
{
    // CC_RETURN_ON_FALSE(ctx != NULL, CC_MASTER_ERR_INVALID_ARG, TAG, "ctx is NULL");

    // /* Check if the property exists. */
    // CC_RETURN_ON_FALSE(property_id <= ctx->property_list_size, CC_MASTER_ERR_INVALID_ARG, TAG,
    //                    "Property (%d) does not exist", property_id);

    // /* Set the page cnt and size. */
    // uint16_t property_size = CTX_PROP.attribute.read_size.static_size;

    // CC_RETURN_ON_FALSE(CTX_PROP.handler.set != NULL &&
    //                        (CTX_PROP.attribute.read_size.static_size || CTX_PROP.attribute.read_size.is_dynamic),
    //                    CC_MASTER_ERR_NOT_SUPPORTED, TAG, "Property (%d) %s is not readable.", property_id,
    //                    CTX_PROP.attribute.name);

    // CC_LOGI(TAG, "Reading Property (%d) %s from all Nodes.", property_id, CTX_PROP.attribute.name);

    // /* Initiate the message. */
    // cc_msg_header_t tx_header = {.action = property_readAll, .property = property_id};
    // uint16_t module_cnt       = 0;

    // /* Flush uart RX buffer. */
    // // uart_flush_input(UART_NUM);

    // /* Send the header. */
    // CC_RETURN_ON_FALSE(cc_master_uart_write(ctx, &tx_header, sizeof(tx_header), NULL) == sizeof(tx_header),
    //                    CC_MASTER_ERR_FAIL, TAG, "Failed to send header");
    // /* Send the module count bytes. */
    // CC_RETURN_ON_FALSE(cc_master_uart_write(ctx, &module_cnt, sizeof(module_cnt), NULL) == sizeof(module_cnt),
    //                    CC_MASTER_ERR_FAIL, TAG, "Failed to send module count bytes");

    // /* Receive the header. */
    // cc_msg_header_t rx_header = {0};
    // ctx->uart.read_timeout_set(ctx->uart_userdata, RX_BYTES_TIMEOUT(sizeof(rx_header)));
    // CC_RETURN_ON_FALSE(cc_master_uart_read(ctx, &rx_header, sizeof(rx_header), NULL) == sizeof(rx_header),
    //                    CC_MASTER_ERR_TIMEOUT, TAG, "Failed to receive header");
    // CC_RETURN_ON_FALSE(tx_header.raw == rx_header.raw, CC_MASTER_ERR_FAIL, TAG, "Header mismatch");

    // /* Receive the module count. */
    // ctx->uart.read_timeout_set(ctx->uart_userdata, RX_BYTES_TIMEOUT(sizeof(module_cnt)));
    // CC_RETURN_ON_FALSE(cc_master_uart_read(ctx, &module_cnt, sizeof(module_cnt), NULL) == sizeof(module_cnt),
    //                    CC_MASTER_ERR_TIMEOUT, TAG, "Failed to receive module count");

    // /* Update the node count. */
    // ctx->master.node_cnt_update(ctx->cc_userdata, module_cnt);

    // /* Receive the data. */
    // for (uint16_t i = 0; i < module_cnt; i++) {
    //     uint8_t property_data[CC_PROPERTY_SIZE_MAX] = {0};

    //     /* Initialize the checksum.*/
    //     uint8_t rx_checksum_calc   = tx_header.raw + (uint8_t)((i + 1) >> 8) + (uint8_t)(i + 1);
    //     uint8_t rx_checksum_actual = 0; /* The actual checksum received. */

    //     if (CTX_PROP.attribute.read_size.is_dynamic) {
    //         ctx->uart.read_timeout_set(ctx->uart_userdata, RX_BYTES_TIMEOUT(2));
    //         CC_RETURN_ON_FALSE(cc_master_uart_read(ctx, (uint8_t *)&property_size, 2, &rx_checksum_calc) == 2,
    //                            CC_MASTER_ERR_TIMEOUT, TAG, "Failed to receive dynamic property size");
    //     }

    //     /* Read the data. */
    //     ctx->uart.read_timeout_set(ctx->uart_userdata, RX_BYTES_TIMEOUT(property_size));
    //     CC_RETURN_ON_FALSE(cc_master_uart_read(ctx, property_data, property_size, &rx_checksum_calc) ==
    //     property_size,
    //                        CC_MASTER_ERR_TIMEOUT, TAG, "Failed to receive read all data");

    //     /* Verify the checksum. */
    //     ctx->uart.read_timeout_set(ctx->uart_userdata, RX_BYTES_TIMEOUT(1));
    //     CC_RETURN_ON_FALSE(cc_master_uart_read(ctx, &rx_checksum_actual, 1, NULL) == 1, CC_MASTER_ERR_TIMEOUT, TAG,
    //                        "Failed to receive checksum");
    //     CC_RETURN_ON_FALSE(rx_checksum_calc == rx_checksum_actual, CC_MASTER_ERR_FAIL, TAG, "Checksum mismatch");

    //     /* Handle the data. */
    //     CTX_PROP.handler.set(i, property_data, &property_size, ctx->cc_userdata);
    // }

    return CC_MASTER_OK;
}

//----------------------------------------------------------------------------------------------------------------------

cc_master_err_t cc_master_prop_write_all(cc_master_ctx_t *ctx, cc_prop_id_t property_id)
{
    // cc_master_err_t ret                         = CC_MASTER_OK; /* Set by CC_GOTO_ON_ERROR macro. */
    // uint16_t property_size                      = 0;
    // uint8_t property_data[CC_PROPERTY_SIZE_MAX] = {0};
    // uint8_t rx_buff[CC_PROPERTY_SIZE_MAX]       = {0};
    // cc_msg_return_code_t msg_rc                 = {.action = returnCode, .rc = CC_RC_SUCCESS};

    // /* Check if the property exists. */
    // CC_RETURN_ON_FALSE(property_id <= ctx->property_list_size, CC_MASTER_ERR_INVALID_ARG, TAG,
    //                    "Property (%d) does not exist", property_id);

    // /* Set the page cnt and size. */
    // property_size = CTX_PROP.attribute.write_size.static_size;

    // CC_RETURN_ON_FALSE(CTX_PROP.handler.get != NULL &&
    //                        (CTX_PROP.attribute.write_size.static_size || CTX_PROP.attribute.write_size.is_dynamic),
    //                    CC_MASTER_ERR_NOT_SUPPORTED, TAG, "Property (%d) %s is not writable.", property_id,
    //                    CTX_PROP.attribute.name);

    // CC_LOGI(TAG, "Writing Property (%d) %s to all Nodes.", property_id, CTX_PROP.attribute.name);

    // CTX_PROP.handler.get(0, property_data, &property_size, ctx->cc_userdata);

    // /* Flush uart RX buffer. */
    // // uart_flush_input(UART_NUM);

    // uint8_t tx_checksum_calc   = 0;
    // uint8_t rx_checksum_actual = 0;

    // /* Initiate the message. */
    // cc_msg_header_t tx_header = {.action = property_writeAll, .property = property_id};

    // /* Send the header. */
    // CC_RETURN_ON_FALSE(cc_master_uart_write(ctx, &tx_header, sizeof(tx_header), &tx_checksum_calc) ==
    // sizeof(tx_header),
    //                    CC_MASTER_ERR_FAIL, TAG, "Failed to send header");

    // /* Send dynamic property size. */
    // if (CTX_PROP.attribute.write_size.is_dynamic) {
    //     CC_RETURN_ON_FALSE(cc_master_uart_write(ctx, &property_size, sizeof(property_size), &tx_checksum_calc) ==
    //                            sizeof(property_size),
    //                        CC_MASTER_ERR_FAIL, TAG, "Failed to send dynamic property size");
    // }

    // /* Send the property data. */
    // CC_RETURN_ON_FALSE(cc_master_uart_write(ctx, property_data, property_size, &tx_checksum_calc) == property_size,
    //                    CC_MASTER_ERR_FAIL, TAG, "Failed to send property data");

    // /* Send the checksum. */
    // CC_RETURN_ON_FALSE(cc_master_uart_write(ctx, &tx_checksum_calc, sizeof(tx_checksum_calc), NULL) ==
    //                        sizeof(tx_checksum_calc),
    //                    CC_MASTER_ERR_FAIL, TAG, "Failed to send checksum");

    // /* Send message return code. */
    // CC_RETURN_ON_FALSE(cc_master_uart_write(ctx, &msg_rc.raw, 1, NULL) == 1, CC_MASTER_ERR_FAIL, TAG,
    //                    "Failed to send message return code");

    // /* Receive the header and compare with the original. */
    // cc_msg_header_t rx_header = {0};
    // ctx->uart.read_timeout_set(ctx->uart_userdata, RX_BYTES_TIMEOUT(sizeof(rx_header)));
    // CC_RETURN_ON_FALSE(cc_master_uart_read(ctx, &rx_header, sizeof(rx_header), NULL) == sizeof(rx_header),
    //                    CC_MASTER_ERR_TIMEOUT, TAG, "Failed to receive header");
    // CC_RETURN_ON_FALSE(rx_header.raw == tx_header.raw, CC_MASTER_ERR_FAIL, TAG, "Header mismatch");

    // /* Receive dynamic size and compare with the original.*/
    // if (CTX_PROP.attribute.write_size.is_dynamic) {
    //     uint16_t rx_property_size = 0;

    //     CC_RETURN_ON_FALSE(cc_master_uart_read(ctx, &rx_property_size, sizeof(rx_property_size), NULL) ==
    //                            sizeof(rx_property_size),
    //                        CC_MASTER_ERR_TIMEOUT, TAG, "Failed to receive property size header");
    //     CC_RETURN_ON_FALSE(rx_property_size == property_size, CC_MASTER_ERR_FAIL, TAG, "Property size mismatch");
    // }

    // /* Receive the data and compare with the original data.*/
    // ctx->uart.read_timeout_set(ctx->uart_userdata, RX_BYTES_TIMEOUT(property_size));
    // CC_RETURN_ON_FALSE(cc_master_uart_read(ctx, rx_buff, property_size, NULL) == property_size,
    // CC_MASTER_ERR_TIMEOUT,
    //                    TAG, "Failed to receive data");
    // CC_RETURN_ON_FALSE(memcmp(property_data, rx_buff, property_size) == 0, CC_MASTER_ERR_FAIL, TAG,
    //                    "TX and RD data mismatch");

    // /* Verify the checksum. */
    // ctx->uart.read_timeout_set(ctx->uart_userdata, RX_BYTES_TIMEOUT(1));
    // CC_RETURN_ON_FALSE(cc_master_uart_read(ctx, &rx_checksum_actual, 1, NULL) == 1, CC_MASTER_ERR_TIMEOUT, TAG,
    //                    "Failed to receive checksum");
    // CC_RETURN_ON_FALSE(tx_checksum_calc == rx_checksum_actual, CC_MASTER_ERR_FAIL, TAG, "Checksum mismatch");

    // /* Receive message return code. */
    // ctx->uart.read_timeout_set(ctx->uart_userdata, RX_BYTES_TIMEOUT(1));
    // CC_RETURN_ON_FALSE(cc_master_uart_read(ctx, &msg_rc.raw, 1, NULL) == 1, CC_MASTER_ERR_TIMEOUT, TAG,
    //                    "Failed to receive message return code");
    // CC_RETURN_ON_FALSE(msg_rc.rc == CC_RC_SUCCESS, CC_MASTER_ERR_FAIL, TAG, "Message returned an error code.");

    // /*Cleanup. */
    // return ret;
    return CC_MASTER_OK;
}

//----------------------------------------------------------------------------------------------------------------------

cc_master_err_t cc_master_prop_write_seq(cc_master_ctx_t *ctx, cc_prop_id_t property_id)
{
    // uint16_t property_size = 0;

    // /* Check if the property exists. */
    // CC_RETURN_ON_FALSE(property_id <= ctx->property_list_size, CC_MASTER_ERR_INVALID_ARG, TAG,
    //                    "Property (%d) does not exist", property_id);

    // /* Set the page cnt and size. */
    // property_size = CTX_PROP.attribute.write_size.static_size;

    // CC_RETURN_ON_FALSE(CTX_PROP.handler.get != NULL &&
    //                        (CTX_PROP.attribute.write_size.static_size || CTX_PROP.attribute.write_size.is_dynamic),
    //                    CC_MASTER_ERR_NOT_SUPPORTED, TAG, "Property (%d) %s is not writable.", property_id,
    //                    CTX_PROP.attribute.name);

    // CC_LOGI(TAG, "Writing Property (%d) %s to selected Nodes.", property_id, CTX_PROP.attribute.name);

    // /* Flush uart RX buffer. */
    // // uart_flush_input(UART_NUM);
    // uint16_t node_idx = 0;
    // bool node_must_be_written;
    // while (
    //     ctx->master.node_exists_and_must_be_written(ctx->cc_userdata, node_idx, property_id, &node_must_be_written))
    //     { uint8_t tx_checksum_calc                    = 0; uint8_t property_data[CC_PROPERTY_SIZE_MAX] = {0};

    //     if (node_must_be_written) {
    //         /* Property needs to be written to current module. */

    //         /* Initiate the message. */
    //         cc_msg_header_t tx_header = {.action = property_writeSequential, .property = property_id};

    //         /* Send the header. */
    //         CC_RETURN_ON_FALSE(cc_master_uart_write(ctx, &tx_header, sizeof(tx_header), &tx_checksum_calc) ==
    //                                sizeof(tx_header),
    //                            CC_MASTER_ERR_FAIL, TAG, "Failed to send header");
    //         /* Get the property data. */
    //         CTX_PROP.handler.get(node_idx, property_data, &property_size, ctx->cc_userdata);

    //         /* Send dynamic property size. */
    //         if (CTX_PROP.attribute.write_size.is_dynamic) {
    //             CC_RETURN_ON_FALSE(cc_master_uart_write(ctx, &property_size, sizeof(property_size),
    //                                                     &tx_checksum_calc) == sizeof(property_size),
    //                                CC_MASTER_ERR_FAIL, TAG, "Failed to send dynamic property size");
    //         }

    //         /* Send the property data. */
    //         CC_RETURN_ON_FALSE(cc_master_uart_write(ctx, property_data, property_size, &tx_checksum_calc) ==
    //                                property_size,
    //                            CC_MASTER_ERR_FAIL, TAG, "Failed to send property data");

    //         /* Send the checksum. */
    //         CC_RETURN_ON_FALSE(cc_master_uart_write(ctx, &tx_checksum_calc, sizeof(tx_checksum_calc), NULL) ==
    //                                sizeof(tx_checksum_calc),
    //                            CC_MASTER_ERR_FAIL, TAG, "Failed to send checksum");
    //     } else {
    //         /* Property do's not needs to be written to current module. */
    //         /* Initiate the message. Send PROPERTY_NONE to skip the module. */
    //         cc_msg_header_t tx_header = {.action = property_writeSequential, .property = 0};

    //         /* Send the header. */
    //         CC_RETURN_ON_FALSE(cc_master_uart_write(ctx, &tx_header, sizeof(tx_header), NULL) == sizeof(tx_header),
    //                            CC_MASTER_ERR_FAIL, TAG, "Failed to send header");
    //     }

    //     /* Increment the node index. */
    //     node_idx++;
    // }

    // /* Send first message return code, this triggers the modules to execute their command. */
    // cc_msg_return_code_t msg_rc = {.action = returnCode, .rc = CC_RC_SUCCESS};
    // CC_RETURN_ON_FALSE(cc_master_uart_write(ctx, &msg_rc.raw, 1, NULL) == 1, CC_MASTER_ERR_FAIL, TAG,
    //                    "Failed to send first message return code");
    // /* Send second message return code, when this data returns, we know the modules have finished their command. */
    // CC_RETURN_ON_FALSE(cc_master_uart_write(ctx, &msg_rc.raw, 1, NULL) == 1, CC_MASTER_ERR_FAIL, TAG,
    //                    "Failed to send second message return code");

    // /* Receive first message return code. */
    // ctx->uart.read_timeout_set(ctx->uart_userdata, RX_BYTES_TIMEOUT(10 * node_idx));
    // CC_RETURN_ON_FALSE(cc_master_uart_read(ctx, &msg_rc.raw, 1, NULL) == 1, CC_MASTER_ERR_TIMEOUT, TAG,
    //                    "Failed to receive message return code 1");
    // CC_RETURN_ON_FALSE(msg_rc.rc == CC_RC_SUCCESS, CC_MASTER_ERR_FAIL, TAG, "Message return code 1 mismatch");

    // /* Receive second message return code. */
    // ctx->uart.read_timeout_set(ctx->uart_userdata, RX_BYTES_TIMEOUT(1 * node_idx));
    // CC_RETURN_ON_FALSE(cc_master_uart_read(ctx, &msg_rc.raw, 1, NULL) == 1, CC_MASTER_ERR_TIMEOUT, TAG,
    //                    "Failed to receive message return code 2");
    // CC_RETURN_ON_FALSE(msg_rc.rc == CC_RC_SUCCESS, CC_MASTER_ERR_FAIL, TAG, "Message return code 2 mismatch");

    return CC_MASTER_OK;
}

//----------------------------------------------------------------------------------------------------------------------

cc_master_err_t cc_master_prop_read(cc_master_ctx_t *ctx, cc_prop_id_t property_id)
{
    CC_RETURN_ON_FALSE(ctx != NULL, CC_MASTER_ERR_INVALID_ARG, TAG, "ctx is NULL");

    /* Check if the property exists. */
    CC_RETURN_ON_FALSE(property_id <= ctx->property_list_size, CC_MASTER_ERR_NOT_SUPPORTED, TAG,
                       "Property (%d) does not exist", property_id);

    /* Check if the property handler supports writing. (We read from the nodes and write to the master. ) */
    CC_RETURN_ON_FALSE(CTX_PROP.handler.set != NULL, CC_MASTER_ERR_NOT_SUPPORTED, TAG,
                       "Property (%d) %s is not readable.", property_id, CTX_PROP.attribute.name);

    CC_LOGI(TAG, "Reading Property (%d) %s from all Nodes.", property_id, CTX_PROP.attribute.name);

    /* Initiate the message. */
    cc_msg_header_t tx_header = {0};
    cc_header_action_set(&tx_header, CC_ACTION_READ);
    cc_header_property_set(&tx_header, property_id);
    // cc_header_node_cnt_set(&tx_header, 0);
    cc_header_parity_set(&tx_header);
    printf("Master transmit header: 0x%02X 0x%02X 0x%02X \n", tx_header.raw[0], tx_header.raw[1], tx_header.raw[2]);

    // /* Flush uart RX buffer. */
    // // uart_flush_input(UART_NUM);

    /* Send the header. */
    CC_RETURN_ON_FALSE(ctx->uart.write(ctx->uart_userdata, tx_header.raw, sizeof(tx_header)) == sizeof(tx_header),
                       CC_MASTER_ERR_FAIL, TAG, "Failed to send header");

    /* Receive the header. */
    cc_msg_header_t rx_header = {0};
    ctx->uart.read_timeout_set(ctx->uart_userdata, RX_BYTES_TIMEOUT(sizeof(rx_header)));
    CC_RETURN_ON_FALSE(ctx->uart.read(ctx->uart_userdata, rx_header.raw, sizeof(rx_header)) == sizeof(rx_header),
                       CC_MASTER_ERR_TIMEOUT, TAG, "Failed to receive header");

    printf("Master received header: 0x%02X 0x%02X 0x%02X \n", rx_header.raw[0], rx_header.raw[1], rx_header.raw[2]);
    /* Check header integrity. */
    CC_RETURN_ON_FALSE(cc_header_parity_check(rx_header), CC_MASTER_ERR_FAIL, TAG, "Header parity invalid");
    CC_RETURN_ON_FALSE(cc_header_action_get(rx_header) == CC_ACTION_READ, CC_MASTER_ERR_FAIL, TAG,
                       "Header action corrupted");
    CC_RETURN_ON_FALSE(cc_header_property_get(rx_header) == property_id, CC_MASTER_ERR_FAIL, TAG,
                       "Header property corrupted");

    /* Update the node count. */
    uint16_t node_cnt = cc_header_node_cnt_get(rx_header);
    ctx->master.node_cnt_update(ctx->cc_userdata, node_cnt);

    /* Receive the data. */
    for (uint16_t i = 0; i < node_cnt; i++) {
        uint8_t property_data[CC_PAYLOAD_SIZE_MAX] = {0};
        size_t data_size                           = CC_PROPERTY_SIZE_MAX;

        /* Read the data. */
        ctx->uart.read_timeout_set(ctx->uart_userdata, RX_BYTES_TIMEOUT(1));

        /* Read data until the end of the message. */
        size_t read_cnt = CC_COBS_OVERHEAD_SIZE;
        do {
            CC_RETURN_ON_FALSE(ctx->uart.read(ctx->uart_userdata, property_data + read_cnt, 1) == 1,
                               CC_MASTER_ERR_TIMEOUT, TAG, "Failed to receive read all data");
            read_cnt++;
        } while (property_data[read_cnt - 1] != 0x00 && read_cnt < CC_PAYLOAD_SIZE_MAX);

        /* Decode the payload. */
        CC_RETURN_ON_FALSE(cc_payload_cobs_decode(property_data, &data_size, property_data + CC_COBS_OVERHEAD_SIZE,
                                                  read_cnt - CC_COBS_OVERHEAD_SIZE),
                           CC_MASTER_ERR_PAYLOAD, TAG, "Failed to decode COBS payload");

        /* Verify the checksum. */
        CC_RETURN_ON_FALSE(cc_checksum_calculate(property_data, data_size) == CC_CHECKSUM_OK, CC_MASTER_ERR_CHECKSUM,
                           TAG, "Payload checksum invalid");
        data_size--; /* Remove checksum from size. */

        /* Handle the data. */
        CTX_PROP.handler.set(i, property_data, &data_size, ctx->cc_userdata);
    }

    return CC_MASTER_OK;
}

//----------------------------------------------------------------------------------------------------------------------