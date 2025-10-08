#pragma once

#include "chain_comm_shared.h"

typedef bool (*cc_master_node_cnt_update_cb_t)(void *userdata, uint16_t node_cnt);
typedef bool (*cc_master_node_exists_and_must_be_written_cb_t)(void *userdata, uint16_t node_idx, uint8_t property,
                                                               bool *must_be_written);
typedef enum {
    CC_MASTER_OK = 0,
    CC_MASTER_ERR_TIMEOUT,
    CC_MASTER_ERR_UART,
    CC_MASTER_ERR_CHECKSUM,
    CC_MASTER_ERR_INVALID_ARG,
    CC_MASTER_ERR_NO_MEM,
    CC_MASTER_ERR_NOT_SUPPORTED,
    CC_MASTER_ERR_FAIL,
} cc_master_err_t;

typedef struct {
    uart_read_cb_t read;
    uart_cnt_readable_cb_t cnt_readable;
    uart_write_cb_t write;
    uart_read_timeout_set_cb_t read_timeout_set;
} cc_master_uart_cb_cfg_t;

typedef struct {
    cc_master_node_cnt_update_cb_t node_cnt_update;
    cc_master_node_exists_and_must_be_written_cb_t node_exists_and_must_be_written;
} cc_master_cb_cfg_t;

typedef struct {
    cc_prop_t *property_list;     /**< List of property handlers and attributes. */
    size_t property_list_size;    /**< Size of property_list. */
    void *cc_userdata;            /**< User data for the property callback functions. */
    cc_master_uart_cb_cfg_t uart; /**< Uart driver callback configurations. */
    void *uart_userdata;          /**< Uart user data to be used by callback functions. */

    cc_master_cb_cfg_t master; /**< Master callback configurations. */
} cc_master_ctx_t;

/**
 * \brief Initializes the chain communication context.
 *
 * This function initializes the chain communication context based on the provided UART driver.
 *
 * \param[inout] ctx Pointer to the #cc_node_ctx_t structure containing the context information.
 * \param[in] uart_cb_cfg Pointer to the UART callback structure.
 * \param[in] uart_userdata Pointer to the UART user data.
 * \param[in] master_cb_cfg Pointer to the master callback structure.
 * \param[in] property_list Pointer to the list of properties.
 * \param[in] property_list_size Size of the property list.
 * \param[in] cc_userdata Pointer to user data for the property callback functions.
 */
void cc_master_init(cc_master_ctx_t *ctx, cc_master_uart_cb_cfg_t *uart_cb_cfg, void *uart_userdata,
                    cc_master_cb_cfg_t *master_cb_cfg, cc_prop_t *property_list, size_t property_list_size,
                    void *cc_userdata);

cc_master_err_t cc_master_prop_read_all(cc_master_ctx_t *ctx, cc_prop_id_t property_id);
cc_master_err_t cc_master_prop_write_all(cc_master_ctx_t *ctx, cc_prop_id_t property_id);
cc_master_err_t cc_master_prop_write_seq(cc_master_ctx_t *ctx, cc_prop_id_t property_id);

/**
 * \brief Reads a property from all nodes.
 *
 * \param[inout] ctx Pointer to the #cc_master_ctx_t structure containing the context information.
 * \param[in] property_id The ID of the property to read.
 *
 * \return CC_MASTER_OK on success, or an error code on failure.
 */
cc_master_err_t cc_master_prop_read(cc_master_ctx_t *ctx, cc_prop_id_t property_id);