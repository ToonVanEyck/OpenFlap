#pragma once

#include "madelink_master.h"

#include "driver/uart.h"
#include "esp_check.h"
#include "esp_log.h"

#define UART_BUF_SIZE (1024)
#define UART_NUM      UART_NUM_1
#define COL_START_PIN (2)
#define TX_COL_PIN    (47)
#define RX_COL_PIN    (48)
#define ROW_START_PIN (6)
#define TX_ROW_PIN    (5)
#define RX_ROW_PIN    (4)

typedef struct {
    uart_port_t uart_num;        /**< UART port number. */
    TickType_t rx_timeout_ticks; /**< Timeout for UART read operations. */
} of_mdl_master_uart_ctx_t;

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Initialize the UART for chain communication.
 *
 * @param[out] uart_ctx UART configuration context.
 * @param[out] uart_cb_cfg UART callback configuration to be filled.
 *
 * @return ESP_OK on success.
 */
esp_err_t of_mdl_master_uart_init(of_mdl_master_uart_ctx_t *uart_ctx, mdl_master_uart_cb_cfg_t *uart_cb_cfg);

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Deinitialize the UART for chain communication.
 *
 * @param[in] uart_ctx UART configuration context.
 *
 * @return ESP_OK on success.
 */
esp_err_t of_mdl_master_uart_deinit(of_mdl_master_uart_ctx_t *uart_ctx);

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Reconfigure the UART pins based on the COL_START_PIN and ROW_START_PIN states.
 *
 * @param controller_is_col_start True if the controller is mounted on top of a module.
 * @param controller_is_row_start True if the controller is connected to another top-con board.
 */
esp_err_t of_mdl_master_uart_reconfigure(bool controller_is_col_start, bool controller_is_row_start);