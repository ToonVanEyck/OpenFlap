#include "openflap_cc_master_uart.h"

#include "hal/gpio_hal.h"
#include "rom/gpio.h"
#include "soc/gpio_sig_map.h"

//======================================================================================================================
//                                                   MACROS a DEFINES
//======================================================================================================================

#define UART_BUF_SIZE (1024)
#define UART_NUM      UART_NUM_1
#define COL_START_PIN (2)
#define TX_COL_PIN    (47)
#define RX_COL_PIN    (45)
#define ROW_START_PIN (6)
#define TX_ROW_PIN    (5)
#define RX_ROW_PIN    (4)

//======================================================================================================================
//                                                   FUNCTION PROTOTYPES
//======================================================================================================================

size_t of_cc_master_uart_read(void *uart_userdata, uint8_t *data, size_t size);
size_t of_cc_master_uart_write(void *uart_userdata, const uint8_t *data, size_t size);
void of_cc_master_uart_read_timeout_set(void *uart_userdata, uint32_t timeout_ms);
void of_cc_master_uart_flush_rx_buff(void *uart_userdata);

//======================================================================================================================
//                                                   PUBLIC FUNCTIONS
//======================================================================================================================

esp_err_t cc_master_uart_init(of_cc_master_uart_ctx_t uart_ctx, cc_master_uart_cb_cfg_t *uart_cb_cfg)
{
    uart_config_t uart_config = {
        .baud_rate  = 115200,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    ESP_RETURN_ON_ERROR(uart_driver_install(UART_NUM, UART_BUF_SIZE, UART_BUF_SIZE, 0, NULL, 0), TAG,
                        "Failed to install UART driver");
    ESP_RETURN_ON_ERROR(uart_param_config(UART_NUM, &uart_config), TAG, "Failed to configure UART parameters");

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << COL_START_PIN) | (1ULL << ROW_START_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&io_conf), TAG, "Failed to configure COL_START_PIN and ROW_START_PIN as inputs");

    /* Configure the context. */
    memset(uart_ctx, 0, sizeof(*uart_ctx));
    uart_ctx->uart_num = UART_NUM;

    /* Configure the callback functions. */
    uart_cb_cfg->read             = of_cc_master_uart_read;
    uart_cb_cfg->write            = of_cc_master_uart_write;
    uart_cb_cfg->read_timeout_set = of_cc_master_uart_read_timeout_set;
    uart_cb_cfg->flush_rx_buff    = of_cc_master_uart_flush_rx_buff;

    return ESP_OK;
}

//----------------------------------------------------------------------------------------------------------------------

esp_err_t of_cc_master_uart_deinit(void)
{
    uart_driver_delete(UART_NUM);
    return ESP_OK;
}

//----------------------------------------------------------------------------------------------------------------------

esp_err_t of_cc_master_uart_reconfigure(bool controller_is_col_start, bool controller_is_row_start)
{
    gpio_reset_pin(RX_COL_PIN);
    gpio_set_direction(RX_COL_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(RX_COL_PIN, GPIO_PULLUP_ONLY);
    gpio_reset_pin(TX_COL_PIN);
    gpio_set_direction(TX_COL_PIN, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(TX_COL_PIN, GPIO_FLOATING);
    gpio_reset_pin(RX_ROW_PIN);
    gpio_set_direction(RX_ROW_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(RX_ROW_PIN, GPIO_PULLUP_ONLY);
    gpio_reset_pin(TX_ROW_PIN);
    gpio_set_direction(TX_ROW_PIN, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(TX_ROW_PIN, GPIO_FLOATING);

    if (controller_is_row_start == controller_is_col_start) {
        /* Both values are the same, either connected in a full display or not connected at all. */
        ESP_RETURN_ON_ERROR(uart_set_pin(UART_NUM, TX_COL_PIN, RX_ROW_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE), TAG,
                            "Failed to set UART pins");

        /* Also connect RX_COL_PIN -> TX_ROW_PIN (direct mirror) */
        gpio_matrix_in(RX_COL_PIN, SIG_IN_FUNC_212_IDX, false);
        gpio_matrix_out(TX_ROW_PIN, SIG_IN_FUNC_212_IDX, false, false);
    } else if (controller_is_col_start && !controller_is_row_start) {
        /* Tx and Rx from column only. */
        ESP_RETURN_ON_ERROR(uart_set_pin(UART_NUM, TX_COL_PIN, RX_COL_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE), TAG,
                            "Failed to set UART pins");
    } else if (!controller_is_col_start && controller_is_row_start) {
        /* Tx and Rx from row only. */
        ESP_RETURN_ON_ERROR(uart_set_pin(UART_NUM, TX_ROW_PIN, RX_ROW_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE), TAG,
                            "Failed to set UART pins");
    }

    return ESP_OK;
}

//======================================================================================================================
//                                                         PRIVATE FUNCTIONS
//======================================================================================================================

size_t of_cc_master_uart_read(void *uart_userdata, uint8_t *data, size_t size)
{
    of_cc_master_uart_ctx_t *ctx = (of_cc_master_uart_ctx_t *)uart_userdata;
    return uart_read_bytes(ctx->uart_num, data, size, ctx->rx_timeout_ticks);
}

//----------------------------------------------------------------------------------------------------------------------

size_t of_cc_master_uart_write(void *uart_userdata, const uint8_t *data, size_t size)
{
    of_cc_master_uart_ctx_t *ctx = (of_cc_master_uart_ctx_t *)uart_userdata;
    return uart_write_bytes(ctx->uart_num, (const char *)data, size);
}

//----------------------------------------------------------------------------------------------------------------------

void of_cc_master_uart_read_timeout_set(void *uart_userdata, uint32_t timeout_ms)
{
    of_cc_master_uart_ctx_t *ctx = (of_cc_master_uart_ctx_t *)uart_userdata;
    ctx->rx_timeout_ticks        = pdMS_TO_TICKS(timeout_ms);
}

//----------------------------------------------------------------------------------------------------------------------

void of_cc_master_uart_flush_rx_buff(void *uart_userdata)
{
    of_cc_master_uart_ctx_t *ctx = (of_cc_master_uart_ctx_t *)uart_userdata;
    uart_flush_input(ctx->uart_num);
}

//----------------------------------------------------------------------------------------------------------------------