#pragma once

#include "chain_comm_common.h"
#include "chain_comm_msg.h"

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

esp_err_t chain_comm_property_read_all(display_t *display, property_id_t property);
esp_err_t chain_comm_property_write_all(display_t *display, property_id_t property);
esp_err_t chain_comm_property_write_seq(display_t *display, property_id_t property);
