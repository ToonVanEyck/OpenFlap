#pragma once

#include "chain_comm_shared.h"

#define CC_MASTER_READ_NODE_ERRORS (-1) /**< Use as property with the master read function to read node errors. */

typedef bool (*cc_master_node_cnt_update_cb_t)(void *userdata, uint16_t node_cnt);
typedef bool (*cc_master_node_exists_and_must_be_written_cb_t)(void *userdata, uint16_t node_idx,
                                                               cc_prop_id_t property_id, bool *must_be_written);
typedef void (*cc_master_node_error_set_cb_t)(void *userdata, uint16_t node_idx, cc_node_err_t error,
                                              cc_node_state_t state);

typedef enum {
    CC_MASTER_OK = 0,
    CC_MASTER_ERR_TIMEOUT,
    CC_MASTER_ERR_UART,
    CC_MASTER_ERR_PAYLOAD,
    CC_MASTER_ERR_CHECKSUM,
    CC_MASTER_ERR_INVALID_ARG,
    CC_MASTER_ERR_NO_MEM,
    CC_MASTER_ERR_NOT_SUPPORTED,
    CC_MASTER_ERR_FAIL,
    CC_MASTER_ERR_WRITE_CB,
    CC_MASTER_ERR_READ_CB,
    CC_MASTER_ERR_COBS_ENC,
    CC_MASTER_ERR_COBS_DEC,
    CC_MASTER_ERR_BROADCAST_CORRUPTED,
    CC_MASTER_ERR_QUEUE_EMPTY,
    CC_MASTER_ERR_QUEUE_FULL,
} cc_master_err_t;

typedef struct {
    uart_read_cb_t read;
    uart_write_cb_t write;
    uart_read_timeout_set_cb_t read_timeout_set;
    uart_flush_rx_buff_cb_t flush_rx_buff;
    uart_wait_tx_done_cb_t wait_tx_done;
} cc_master_uart_cb_cfg_t;

typedef struct {
    cc_master_node_cnt_update_cb_t node_cnt_update;
    cc_master_node_exists_and_must_be_written_cb_t node_exists_and_must_be_written;
    cc_master_node_error_set_cb_t node_error_set;
} cc_master_cb_cfg_t;

typedef enum {
    CC_MASTER_QUEUE_STATE_UNDEFINED = 0,
    CC_MASTER_QUEUE_STATE_READ,
    CC_MASTER_QUEUE_STATE_WRITE,
    CC_MASTER_QUEUE_STATE_SYNC_ACK,
    CC_MASTER_QUEUE_STATE_SYNC_COMMIT,
} cc_master_queue_state_t;

typedef struct {
    cc_master_queue_state_t state;
    cc_prop_id_t prop_id;
    cc_prop_id_t original_prop_id;
    uint16_t node_cnt;
    bool broadcast;
    uint8_t attempt_cnt;
} cc_master_queued_action_t;

typedef struct {
    cc_prop_t *prop_list;         /**< List of property handlers and attributes. */
    size_t prop_list_size;        /**< Size of prop_list. */
    void *prop_userdata;          /**< User data for the property callback functions. */
    cc_master_uart_cb_cfg_t uart; /**< Uart driver callback configurations. */
    void *uart_userdata;          /**< Uart user data to be used by callback functions. */

    cc_master_cb_cfg_t master; /**< Master callback configurations. */

    cc_master_queued_action_t queued; /**< Currently queued action. */
} cc_master_ctx_t;

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Initializes the chain communication context.
 *
 * This function initializes the chain communication context based on the provided UART driver.
 *
 * \param[inout] ctx Pointer to the #cc_node_ctx_t structure containing the context information.
 * \param[in] uart_cb_cfg Pointer to the UART callback structure.
 * \param[in] uart_userdata Pointer to the UART user data.
 * \param[in] master_cb_cfg Pointer to the master callback structure.
 * \param[in] prop_list Pointer to the list of properties.
 * \param[in] prop_list_size Size of the property list.
 * \param[in] prop_userdata Pointer to user data for the property callback functions.
 */
void cc_master_init(cc_master_ctx_t *ctx, cc_master_uart_cb_cfg_t *uart_cb_cfg, void *uart_userdata,
                    cc_master_cb_cfg_t *master_cb_cfg, cc_prop_t *prop_list, size_t prop_list_size,
                    void *prop_userdata);

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Reads a property from all nodes.
 *
 * \param[inout] ctx Pointer to the #cc_master_ctx_t structure containing the context information.
 * \param[in] property_id The ID of the property to read. Use CC_MASTER_READ_NODE_ERRORS to read node errors.
 *
 * \return CC_MASTER_OK on success, or an error code on failure.
 */
cc_master_err_t cc_master_prop_read(cc_master_ctx_t *ctx, cc_prop_id_t property_id);

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Writes a property from all nodes.
 *
 * \param[inout] ctx Pointer to the #cc_master_ctx_t structure containing the context information.
 * \param[in] property_id The ID of the property to write.
 * \param[in] node_cnt The number of nodes to write the property to. (Must be 0 for broadcast.)
 * \param[in] staged_write If true, the property will be staged and not applied immediately.
 * \param[in] broadcast If true, the property read from node 1 will be written to all nodes.
 *
 * \return CC_MASTER_OK on success, or an error code on failure.
 */
cc_master_err_t cc_master_prop_write(cc_master_ctx_t *ctx, cc_prop_id_t property_id, uint16_t node_cnt,
                                     bool staged_write, bool broadcast);

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Performs a SYNC ACK action with all nodes.
 *
 * \param[inout] ctx Pointer to the #cc_master_ctx_t structure containing the context information.
 * \param[out] node_errors_present Pointer where node errors will be reported.
 *
 * \return CC_MASTER_OK on success, or an error code on failure.
 */
cc_master_err_t cc_master_sync_ack(cc_master_ctx_t *ctx, cc_sync_error_code_t *node_errors_present);

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Queue a read request for a property from all nodes.
 *
 * \param[inout] ctx Pointer to the #cc_master_ctx_t structure containing the context information.
 * \param[in] property_id The ID of the property to read. Use CC_MASTER_READ_NODE_ERRORS to read node errors.
 *
 * \return CC_MASTER_OK on success, CC_MASTER_ERR_FAIL if there was already a queued action.
 */
cc_master_err_t cc_master_queue_prop_read(cc_master_ctx_t *ctx, cc_prop_id_t property_id);

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Queue a write request for a property from all nodes.
 *
 * \param[inout] ctx Pointer to the #cc_master_ctx_t structure containing the context information.
 * \param[in] property_id The ID of the property to write.
 * \param[in] node_cnt The number of nodes to write the property to. (Must be 0 for broadcast.)
 * \param[in] broadcast If true, the property read from node 1 will be written to all nodes.
 *
 * \return CC_MASTER_OK on success, CC_MASTER_ERR_FAIL if there was already a queued action.
 */
cc_master_err_t cc_master_queue_prop_write(cc_master_ctx_t *ctx, cc_prop_id_t property_id, uint16_t node_cnt,
                                           bool broadcast);

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Handles the queued chain communication requests.
 *
 * This function executes the chain communication master FSM based on the provided context. As long as the function
 * returns an error code and a next_tick_ms greater than 0, it should be called again to continue processing.
 *
 * \param[inout] ctx Pointer to the #cc_master_ctx_t structure containing the context information.
 * \param[out] next_tick_ms If this function does not return CC_MASTER_ERR_TIMEOUT and this value is not 0, call this
 * function again after the specified milliseconds.
 *
 * \return CC_MASTER_OK on success, or an error code on failure.
 */
cc_master_err_t cc_master_communication_handler(cc_master_ctx_t *ctx, uint32_t *next_tick_ms);