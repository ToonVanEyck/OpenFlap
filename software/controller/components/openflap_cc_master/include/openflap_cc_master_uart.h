#pragma once

#include "chain_comm_master.h"

#include "driver/uart.h"
#include "esp_check.h"
#include "esp_log.h"

typedef struct {
    uart_port_t uart_num;        /**< UART port number. */
    TickType_t rx_timeout_ticks; /**< Timeout for UART read operations. */
} of_cc_master_uart_ctx_t;

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Initialize the UART for chain communication.
 *
 * @param[out] uart_ctx UART configuration context.
 * @param[out] uart_cb_cfg UART callback configuration to be filled.
 *
 * @return ESP_OK on success.
 */
esp_err_t cc_master_uart_init(of_cc_master_uart_ctx_t uart_ctx, cc_master_uart_cb_cfg_t *uart_cb_cfg);

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Deinitialize the UART for chain communication.
 */
esp_err_t of_cc_master_uart_deinit(void);

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Reconfigure the UART pins based on the COL_START_PIN and ROW_START_PIN states.
 *
 * @param controller_is_col_start True if the controller is mounted on top of a module.
 * @param controller_is_row_start True if the controller is connected to another top-con board.
 */
esp_err_t of_cc_master_uart_reconfigure(bool controller_is_col_start, bool controller_is_row_start);