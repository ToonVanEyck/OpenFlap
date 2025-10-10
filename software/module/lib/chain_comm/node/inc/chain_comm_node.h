#pragma once

#include "chain_comm_shared.h"

#define GENERATE_ENUM_VALUE(ENUM, NAME) ENUM,
#define GENERATE_ENUM_NAME(ENUM, NAME)  NAME,

#define CC_NODE_STATE_GENERATOR(GENERATOR)                                                                             \
    GENERATOR(CC_NODE_STATE_RX_HEADER, "Receive Header")                                                               \
    GENERATOR(CC_NODE_STATE_ERROR, "Error")                                                                            \
    GENERATOR(CC_NODE_STATE_TIMEOUT, "Timeout")                                                                        \
    GENERATOR(CC_NODE_STATE_DEC_NODE_CNT, "Decrement Node Cnt")                                                        \
    GENERATOR(CC_NODE_STATE_TX_PROP, "Transmit Property Data")                                                         \
    GENERATOR(CC_NODE_STATE_RX_PROP, "Receive Property Data")
typedef enum { CC_NODE_STATE_GENERATOR(GENERATE_ENUM_VALUE) } cc_node_state_t;

typedef struct {
    uart_read_cb_t read;
    uart_cnt_readable_cb_t cnt_readable;
    uart_write_cb_t write;
    uart_cnt_writable_cb_t cnt_writable;
    uart_tx_buff_empty_cb_t tx_buff_empty;
    uart_is_busy_cb_t is_busy;
} cc_node_uart_cb_cfg_t;

#define CC_NODE_ERROR_GENERATOR(GENERATOR)                                                                             \
    GENERATOR(CC_NODE_ERR_NONE = 0, "None")                                                                            \
    GENERATOR(CC_NODE_ERR_TIMEOUT, "Timeout")                                                                          \
    GENERATOR(CC_NODE_ERR_COBS_ENC, "COBS Encoding")                                                                   \
    GENERATOR(CC_NODE_ERR_COBS_DEC, "COBS Decoding")                                                                   \
    GENERATOR(CC_NODE_ERR_CHECKSUM, "Checksum")                                                                        \
    GENERATOR(CC_NODE_ERR_READ_NOT_SUPPORTED, "Read Not Supported")                                                    \
    GENERATOR(CC_NODE_ERR_WRITE_NOT_SUPPORTED, "Write Not Supported")                                                  \
    GENERATOR(CC_NODE_ERR_PROP_NOT_SUPPORTED, "Property Not Supported")                                                \
    GENERATOR(CC_NODE_ERR_WRITE_CB, "Write Callback")                                                                  \
    GENERATOR(CC_NODE_ERR_READ_CB, "Read Callback")                                                                    \
    GENERATOR(CC_NODE_ERR_HEADER_PARITY, "Header Parity")                                                              \
    GENERATOR(CC_NODE_ERR_INVALID_STATE, "Invalid State")
typedef enum { CC_NODE_ERROR_GENERATOR(GENERATE_ENUM_VALUE) } cc_node_err_t;

/**
 * \brief Chain-comm context object.
 */
typedef struct {
    cc_prop_t *property_list;   /**< List of property handlers and attributes. */
    size_t property_list_size;  /**< Size of property_list. */
    void *cc_userdata;          /**< User data for the property callback functions. */
    cc_node_uart_cb_cfg_t uart; /**< Uart driver callback configurations. */
    void *uart_userdata;        /**< Uart user data to be used by callback functions. */

    cc_node_state_t state;      /**< The current state of the FSM managing the protocol. */
    cc_node_state_t next_state; /**< The next state of the FSM managing the protocol. */

    uint32_t timeout_tick_cnt; /**< Counter for determining timeout. */
    uint32_t last_tick_ms;     /**< Last tick in milliseconds, used for timeout calculation. */

    cc_node_err_t last_error;    /**< Last error code. */
    cc_node_state_t error_state; /**< State in which the last error occurred. */

    cc_msg_header_t header;   /**< The header of the current message. */
    cc_action_t action;       /**< The action of the current message (from header). */
    bool staged_write;        /**< The staging bit of the current message (from header). */
    cc_prop_id_t property_id; /**< The ID of the current property (from header). */
    int16_t node_cnt;         /**< The node counter (from header). */

    uint8_t data_cnt; /**< The number of bytes handled in the current state. */

    uint8_t property_data[CC_PAYLOAD_SIZE_MAX]; /**< The data of the current property to be written or read. */
    size_t property_size;                       /**< The size of the current property. */

    // cc_ret_code_t msg_rc;                        /**< return code for acknowledgment. */
    // uint8_t checksum_rx_calc;                    /**< Running checksum of received data. */
    // uint8_t checksum_tx_calc;                    /**< Running checksum of transmitted data. */
    // uint16_t writeSeq_packet_cnt;                /**< Counter for the number of packets in the write sequence. */
    // uint16_t writeSeq_property_size; /**< Copy of this modules property to be written, to restore at execution. */
    // cc_msg_header_t writeSeq_header; /**< Copy of the write sequence message header, to restore at execution. */
} cc_node_ctx_t;

/**
 * \brief Initializes the chain communication context.
 *
 * This function initializes the chain communication context based on the provided UART driver.
 *
 * \param[inout] ctx Pointer to the #cc_node_ctx_t structure containing the context information.
 * \param[in] uart_cb_cfg Pointer to the UART callback structure.
 * \param[in] uart_userdata Pointer to the UART user data.
 * \param[in] property_list Pointer to the list of properties.
 * \param[in] property_list_size Size of the property list.
 * \param[in] cc_userdata Pointer to user data for the property callback functions.
 */
void cc_node_init(cc_node_ctx_t *ctx, const cc_node_uart_cb_cfg_t *uart_cb, void *uart_userdata,
                  cc_prop_t *property_list, size_t property_list_size, void *cc_userdata);

/**
 * \brief Executes the chain communication based on the provided context.
 *
 * This function handles the execution of chain communication based on the provided context.
 * It checks the state of the context and calls the corresponding state handler.
 *
 * \param[inout] ctx Pointer to the #cc_node_ctx_t structure containing the context information.
 * \param[in] tick_ms The current tick in milliseconds.
 *
 * \return True if data needs to be transmitted, false otherwise.
 */
void cc_node_tick(cc_node_ctx_t *ctx, uint32_t tick_ms);

/**
 * \brief Checks if the chain communication is busy.
 * The chain communication is considered busy if the state is not #rxHeader or the uart driver is busy.
 *
 * \param[inout] ctx Pointer to the #cc_node_ctx_t structure containing the context information.
 *
 * \return True if the chain communication is busy, otherwise false.
 */
bool cc_node_is_busy(cc_node_ctx_t *ctx);