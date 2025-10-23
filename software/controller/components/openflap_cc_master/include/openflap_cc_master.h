/**
 * \file openflap_cc_master.h
 *
 * The openflap chain communication master component has the goal to synchronize the internal node model with the actual
 * nodes on the chain comm bus.
 *
 * A task is implemented to wait for a desynchronization event from the model. Once the event is received, the task will
 * check with the model which properties need to be synchronized. The task will then read or write the properties
 * from/to the nodes on the chain comm bus. Once all properties are synchronized, the task will signal the model that
 * the synchronization is complete.
 */

#pragma once

#include "chain_comm_master.h"
#include "openflap_cc_master_uart.h"

#include "openflap_properties.h"

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Check if the masters model requires a property to be synchronized.
 *
 * \param[in] model_userdata Pointer to model user data.
 * \param[in] property_id The property to check for required synchronization.
 *
 * \retval CC_ACTION_READ if the master must read the property from all nodes.
 * \retval CC_ACTION_WRITE if the master must write the property to all nodes.
 * \retval CC_ACTION_BROADCAST if the master must broadcast the property to all nodes.
 * \retval -1 if no synchronization is required.
 */
typedef cc_action_t (*of_cc_master_model_sync_required_cb)(void *model_userdata, cc_prop_id_t property_id);

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Indicate to the model that synchronization of a property has been completed.
 *
 * \param[in] model_userdata Pointer to model user data.
 * \param[in] property_id The property that has been synchronized.
 */
typedef void (*of_cc_master_model_sync_done_cb)(void *model_userdata, cc_prop_id_t property_id);

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Chain communication master callback configuration structure.
 */
typedef struct {
    /**Callback to update the node count. */
    cc_master_node_cnt_update_cb_t node_cnt_update;
    /**Callback to check if a node exists and must be written. */
    cc_master_node_exists_and_must_be_written_cb_t node_exists_and_must_be_written;
    /**Callback to set a node error. */
    cc_master_node_error_set_cb_t node_error_set;
    /**Callback to check if the model requires synchronization of a certain property. */
    of_cc_master_model_sync_required_cb model_sync_required;
    /**Callback to indicate that synchronization is done. */
    of_cc_master_model_sync_done_cb model_sync_done;
} of_cc_master_cb_cfg_t;

//----------------------------------------------------------------------------------------------------------------------

typedef struct {
    cc_master_ctx_t cc_master; /**< Chain communication master context. */
    void *model_userdata;      /**< Model user data. */

    TaskHandle_t task;               /**< Task handle. */
    EventGroupHandle_t event_handle; /**< Event group handle. */

    of_cc_master_uart_ctx_t uart_ctx; /**< UART context. */

    const uint16_t *node_cnt_ref; /**< A pointer to where the node count is stored. */

    /** Callback to check if the model requires synchronization. */
    of_cc_master_model_sync_required_cb model_sync_required;
    of_cc_master_model_sync_done_cb model_sync_done; /**< Callback to indicate that synchronization is done. */
} of_cc_master_ctx_t;

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Initialize the chain communication.
 *
 * \param[in] ctx The chain communication context.
 * \param[in] model_userdata The model user data. Used as userdata for the property handlers.
 * \param[in] of_master_cb_cfg The chain communication master callback configuration.
 * \param[in] node_cnt_ref A pointer to where the current node count is stored.
 *
 * \retval ESP_OK The chain communication was successfully initialized.
 * \retval ESP_ERR_INVALID_ARG The context or model user data is NULL.
 * \retval ESP_FAIL The chain communication failed to initialize.
 */
esp_err_t of_cc_master_init(of_cc_master_ctx_t *ctx, void *model_userdata, of_cc_master_cb_cfg_t *of_master_cb_cfg,
                            const uint16_t *node_cnt_ref);

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Destroy the chain communication.
 *
 * \param[in] ctx The chain communication context.
 *
 * \retval ESP_OK The chain communication was successfully destroyed.
 * \retval ESP_ERR_INVALID_ARG The context is NULL.
 * \retval ESP_FAIL The chain communication failed to destroy.
 */
esp_err_t chain_comm_destroy(of_cc_master_ctx_t *ctx);

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Signal the chain communication task that the model and actual model require synchronization.
 *
 * \param[in] ctx The chain communication context.
 * \param[in] timeout_ms The timeout in milliseconds to wait for synchronization. (0 for no wait)
 *
 * \retval ESP_OK The model is synchronized.
 * \retval ESP_ERR_TIMEOUT The model could not be synchronized within the timeout.
 */
esp_err_t of_cc_master_synchronize(of_cc_master_ctx_t *ctx, uint32_t timeout_ms);