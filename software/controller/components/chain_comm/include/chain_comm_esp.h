#pragma once

#include "chain_comm_common.h"

/**
 * \brief Initialize the chain communication.
 *
 * \param[in] ctx The chain communication context.
 * \param[in] display The display context.
 *
 * \retval ESP_OK The chain communication was successfully initialized.
 * \retval ESP_ERR_INVALID_ARG The context or display is NULL.
 * \retval ESP_FAIL The chain communication failed to initialize.
 */
esp_err_t chain_comm_init(chain_comm_ctx_t *ctx, display_t *display);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Reconfigure the chain communication IOs based on the COL_START_PIN and ROW_START_PIN states.
 *
 * \param[in] controller_is_col_start True if the controller is mounted on top of a module.
 * \param[in] controller_is_row_start True if the controller is connected to another top-con board.
 *
 * \retval ESP_OK The chain communication IOs were successfully reconfigured.
 * \retval ESP_FAIL The chain communication IOs failed to reconfigure.
 */
esp_err_t chain_comm_io_reconfigure(bool controller_is_col_start, bool controller_is_row_start);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Destroy the chain communication.
 *
 * \param[in] ctx The chain communication context.
 *
 * \retval ESP_OK The chain communication was successfully destroyed.
 * \retval ESP_ERR_INVALID_ARG The context is NULL.
 * \retval ESP_FAIL The chain communication failed to destroy.
 */
esp_err_t chain_comm_destroy(chain_comm_ctx_t *ctx);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Read a property from all modules.
 *
 * \param[in] display The display which owns the properties to read from.
 * \param[in] property_id The property id to read.
 *
 * \retval ESP_OK The properties were successfully read.
 * \retval ESP_ERR_INVALID_ARG The display is NULL.
 * \retval ESP_FAIL The properties failed to read.
 */
esp_err_t chain_comm_property_read_all(display_t *display, property_id_t property_id);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Write a property with the same value to all modules.
 *
 * \param[in] display The display which owns the properties to write to.
 * \param[in] property_id The property id to write.
 *
 * \retval ESP_OK The properties were successfully written.
 * \retval ESP_ERR_INVALID_ARG The display is NULL.
 * \retval ESP_FAIL The properties failed to write.
 */
esp_err_t chain_comm_property_write_all(display_t *display, property_id_t property_id);

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Write a property with different values to all modules in sequence.
 *
 * \param[in] display The display which owns the properties to write to.
 * \param[in] property_id The property id to write.
 *
 * \retval ESP_OK The properties were successfully written.
 * \retval ESP_ERR_INVALID_ARG The display is NULL.
 * \retval ESP_FAIL The properties failed to write.
 */
esp_err_t chain_comm_property_write_seq(display_t *display, property_id_t property_id);
