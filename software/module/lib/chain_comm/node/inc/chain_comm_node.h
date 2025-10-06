#pragma once

#include "chain_comm_shared.h"

#define GENERATE_STATE_ENUM(ENUM, NAME) ENUM,
#define GENERATE_STATE_NAME(ENUM, NAME) NAME,

#define CHAIN_COMM_STATE(GENERATOR)                                                                                    \
    GENERATOR(rxHeader, "rxHeader")                                                                                    \
    GENERATOR(rxSize, "rxSize")                                                                                        \
    GENERATOR(readAll_rxCnt, "readAll_rxCnt")                                                                          \
    GENERATOR(readAll_rxData, "readAll_rxData")                                                                        \
    GENERATOR(readAll_txSize, "readAll_txSize")                                                                        \
    GENERATOR(readAll_txData, "readAll_txData")                                                                        \
    GENERATOR(writeAll_rxData, "writeAll_rxData")                                                                      \
    GENERATOR(writeSeq_rxData, "writeSeq_rxData")                                                                      \
    GENERATOR(writeAll_rxAck, "writeAll_rxAck")

typedef enum { CHAIN_COMM_STATE(GENERATE_STATE_ENUM) } cc_node_state_t;

typedef struct {
    uart_read_cb_t read;
    uart_cnt_readable_cb_t cnt_readable;
    uart_write_cb_t write;
    uart_cnt_writable_cb_t cnt_writable;
    uart_tx_buff_empty_cb_t tx_buff_empty;
    uart_is_busy_cb_t is_busy;
} cc_node_uart_cb_cfg_t;

/**
 * \brief Chain-comm context object.
 */
typedef struct {
    cc_prop_t *property_list;   /**< List of property handlers and attributes. */
    size_t property_list_size;  /**< Size of property_list. */
    void *cc_userdata;          /**< User data for the property callback functions. */
    cc_node_uart_cb_cfg_t uart; /**< Uart driver callback configurations. */
    void *uart_userdata;        /**< Uart user data to be used by callback functions. */

    cc_node_state_t state;                       /**< The current state of the FSM managing the protocol. */
    cc_msg_header_t header;                      /**< The header of the current message. */
    uint8_t data_cnt;                            /**< The number of bytes handled in the current state. */
    uint16_t index;                              /**< The index counter of the module in the display. */
    uint8_t property_data[CC_PROPERTY_SIZE_MAX]; /**< The data of the current property to be written or read. */
    cc_ret_code_t msg_rc;                        /**< return code for acknowledgment. */
    uint32_t timeout_tick_cnt;                   /**< Counter for determining timeout. */
    uint16_t property_size;                      /**< The size of the current property. */
    uint8_t checksum_rx_calc;                    /**< Running checksum of received data. */
    uint8_t checksum_tx_calc;                    /**< Running checksum of transmitted data. */
    uint16_t writeSeq_packet_cnt;                /**< Counter for the number of packets in the write sequence. */
    uint16_t writeSeq_property_size; /**< Copy of this modules property to be written, to restore at execution. */
    cc_msg_header_t writeSeq_header; /**< Copy of the write sequence message header, to restore at execution. */
    uint32_t last_tick_ms;           /**< Last tick in milliseconds, used for timeout calculation. */
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
bool cc_node_tick(cc_node_ctx_t *ctx, uint32_t tick_ms);

/**
 * \brief Checks if the chain communication is busy.
 * The chain communication is considered busy if the state is not #rxHeader or the uart driver is busy.
 *
 * \param[inout] ctx Pointer to the #cc_node_ctx_t structure containing the context information.
 *
 * \return True if the chain communication is busy, otherwise false.
 */
bool cc_node_is_busy(cc_node_ctx_t *ctx);