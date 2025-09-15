#include "chain_comm_esp.h"
#include "driver/uart.h"
#include "esp_check.h"
#include "esp_log.h"
#include "property_handler.h"

#include <string.h>

#define TAG "CHAIN_COMM"

#define CHAIN_COMM_TASK_SIZE 6000
#define CHAIN_COMM_TASK_PRIO 5

#define UART_BUF_SIZE (1024)
#define UART_NUM      UART_NUM_1
#define TX_PIN        (17)
#define RX_PIN        (16)

#define RX_BYTES_TIMEOUT(_byte_cnt) (pdMS_TO_TICKS((_byte_cnt) * 50))

static void chain_comm_task(void *arg);
static int chain_comm_write_bytes(uart_port_t uart_num, const void *src, size_t size, uint8_t *checksum);
static int chain_comm_read_bytes(uart_port_t uart_num, void *buf, uint32_t length, TickType_t ticks_to_wait,
                                 uint8_t *checksum);

//---------------------------------------------------------------------------------------------------------------------

esp_err_t chain_comm_init(chain_comm_ctx_t *ctx, display_t *display)
{
    ESP_RETURN_ON_FALSE(ctx != NULL, ESP_ERR_INVALID_ARG, TAG, "Chain-comm context is NULL");
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "Display is NULL");

    ctx->display = display;

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
    ESP_RETURN_ON_ERROR(uart_set_pin(UART_NUM, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE), TAG,
                        "Failed to set UART pins");

    ESP_RETURN_ON_FALSE(
        xTaskCreate(chain_comm_task, "chain_comm_task", CHAIN_COMM_TASK_SIZE, ctx, CHAIN_COMM_TASK_PRIO, &ctx->task),
        ESP_FAIL, TAG, "Failed to create chain comm task");

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t chain_comm_destroy(chain_comm_ctx_t *ctx)
{
    ESP_RETURN_ON_FALSE(ctx != NULL, ESP_ERR_INVALID_ARG, TAG, "Chain-comm context is NULL");

    uart_driver_delete(UART_NUM);

    vTaskDelete(ctx->task);

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t chain_comm_property_read_all(display_t *display, property_id_t property_id)
{
    esp_err_t ret = ESP_OK; /* Set by ESP_GOTO_ON_ERROR macro. */

    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "display is NULL");

    const char *property_name = chain_comm_property_name_by_id(property_id);
    ESP_RETURN_ON_FALSE(property_name != NULL, ESP_ERR_INVALID_ARG, TAG, "property %d invalid", property_id);

    const property_handler_t *property_handler = property_handler_get_by_id(property_id);
    ESP_RETURN_ON_FALSE(property_handler != NULL, ESP_ERR_NOT_SUPPORTED, TAG, "No handler for [%s] property.",
                        property_name);
    ESP_RETURN_ON_FALSE(property_handler->from_binary != NULL, ESP_ERR_NOT_SUPPORTED, TAG,
                        "[%s] Property is not readable.", property_name);

    ESP_LOGI(TAG, "Reading all [%s] property", property_name);

    /* Initiate the message. */
    chain_comm_msg_header_t tx_header = {.action = property_readAll, .property = property_id};
    uint16_t module_cnt               = 0;

    /* Flush uart RX buffer. */
    uart_flush_input(UART_NUM);

    /* Send the header. */
    ESP_RETURN_ON_FALSE(chain_comm_write_bytes(UART_NUM, &tx_header, sizeof(tx_header), NULL) == sizeof(tx_header),
                        ESP_FAIL, TAG, "Failed to send header");
    /* Send the module count bytes. */
    ESP_RETURN_ON_FALSE(chain_comm_write_bytes(UART_NUM, &module_cnt, sizeof(module_cnt), NULL) == sizeof(module_cnt),
                        ESP_FAIL, TAG, "Failed to send module count bytes");

    /* Receive the header. */
    chain_comm_msg_header_t rx_header = {0};
    ESP_RETURN_ON_FALSE(chain_comm_read_bytes(UART_NUM, &rx_header, sizeof(rx_header),
                                              RX_BYTES_TIMEOUT(sizeof(rx_header)), NULL) == sizeof(rx_header),
                        ESP_FAIL, TAG, "Failed to receive header");
    ESP_RETURN_ON_FALSE(tx_header.raw == rx_header.raw, ESP_FAIL, TAG, "Header mismatch");

    /* Receive the module count. */
    ESP_RETURN_ON_FALSE(chain_comm_read_bytes(UART_NUM, &module_cnt, sizeof(module_cnt),
                                              RX_BYTES_TIMEOUT(sizeof(module_cnt)), NULL) == sizeof(module_cnt),
                        ESP_FAIL, TAG, "Failed to receive module count");

    /* Resize the display. */
    display_resize(display, module_cnt);

    /* Receive the data. */
    for (uint16_t i = 0; ret == ESP_OK && i < module_cnt; i++) {
        /* Get the module. */
        module_t *module = display_module_get(display, i);
        assert(module != NULL);

        /* Get the property read attributes.  */
        const chain_comm_binary_attributes_t *chain_comm_read_attr =
            chain_comm_property_read_attributes_get(property_handler->id);

        /* Set the page cnt and size. */
        uint16_t property_size = chain_comm_read_attr->static_property_size;

        /* Initialize the checksum.*/
        uint8_t rx_checksum_calc   = tx_header.raw + (uint8_t)((i + 1) >> 8) + (uint8_t)(i + 1);
        uint8_t rx_checksum_actual = 0; /* The actual checksum received. */

        if (chain_comm_read_attr->dynamic_property_size) {
            ESP_RETURN_ON_FALSE(chain_comm_read_bytes(UART_NUM, (uint8_t *)&property_size, 2, RX_BYTES_TIMEOUT(2),
                                                      &rx_checksum_calc) == 2,
                                ESP_FAIL, TAG, "Failed to receive dynamic property size");
        }

        /* Allocate memory for the data. */
        uint8_t *rx_buff = malloc(property_size);
        ESP_RETURN_ON_FALSE(rx_buff != NULL, ESP_ERR_NO_MEM, TAG, "Memory allocation failed");
        /* Read the data. */
        ESP_GOTO_ON_FALSE(chain_comm_read_bytes(UART_NUM, rx_buff, property_size, RX_BYTES_TIMEOUT(property_size),
                                                &rx_checksum_calc) == property_size,
                          ESP_FAIL, exit_loop_cleanup_buffer, TAG, "Failed to receive read all data");

        /* Verify the checksum. */
        ESP_GOTO_ON_FALSE(chain_comm_read_bytes(UART_NUM, &rx_checksum_actual, 1, RX_BYTES_TIMEOUT(1), NULL) == 1,
                          ESP_FAIL, exit_loop_cleanup_buffer, TAG, "Failed to receive checksum");
        ESP_GOTO_ON_FALSE(rx_checksum_calc == rx_checksum_actual, ESP_FAIL, exit_loop_cleanup_buffer, TAG,
                          "Checksum mismatch");

        /* Handle the data. */
        ESP_GOTO_ON_ERROR(property_handler->from_binary(module, rx_buff, property_size), exit_loop_cleanup_buffer, TAG,
                          "Failed to convert read all data");
    exit_loop_cleanup_buffer:
        free(rx_buff);
    }

    return ret;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t chain_comm_property_write_all(display_t *display, property_id_t property_id)
{
    esp_err_t ret                       = ESP_OK; /* Set by ESP_GOTO_ON_ERROR macro. */
    uint8_t *property_data              = NULL;
    uint8_t *rx_buff                    = NULL;
    uint16_t property_size              = 0;
    chain_comm_msg_return_code_t msg_rc = {.action = returnCode, .rc = CHAIN_COMM_RC_SUCCESS};

    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "display is NULL");

    const char *property_name = chain_comm_property_name_by_id(property_id);
    ESP_RETURN_ON_FALSE(property_name != NULL, ESP_ERR_INVALID_ARG, TAG, "property %d invalid", property_id);

    const property_handler_t *property_handler = property_handler_get_by_id(property_id);
    ESP_RETURN_ON_FALSE(property_handler != NULL, ESP_ERR_NOT_SUPPORTED, TAG, "No handler for [%s] property.",
                        property_name);
    ESP_RETURN_ON_FALSE(property_handler->to_binary != NULL, ESP_ERR_NOT_SUPPORTED, TAG,
                        "[%s] Property is not writable.", property_name);

    module_t *module = display_module_get(display, 0);
    ESP_RETURN_ON_FALSE(module != NULL, ESP_ERR_INVALID_ARG, TAG, "No modules in display");

    ESP_LOGI(TAG, "Writing all [%s] property", property_name);

    /* Flush uart RX buffer. */
    uart_flush_input(UART_NUM);

    uint8_t tx_checksum_calc   = 0;
    uint8_t rx_checksum_actual = 0;

    /* Initiate the message. */
    chain_comm_msg_header_t tx_header = {.action = property_writeAll, .property = property_id};

    /* Send the header. */
    ESP_RETURN_ON_FALSE(chain_comm_write_bytes(UART_NUM, &tx_header, sizeof(tx_header), &tx_checksum_calc) ==
                            sizeof(tx_header),
                        ESP_FAIL, TAG, "Failed to send header");

    /* Set the property data of the message. */
    const chain_comm_binary_attributes_t *chain_comm_write_attr =
        chain_comm_property_write_attributes_get(property_handler->id);

    ESP_RETURN_ON_ERROR(property_handler->to_binary(&property_data, &property_size, module), TAG,
                        "Failed to get property size");

    /* Send dynamic property size. */
    if (chain_comm_write_attr->dynamic_property_size) {
        ESP_GOTO_ON_FALSE(chain_comm_write_bytes(UART_NUM, &property_size, sizeof(property_size), &tx_checksum_calc) ==
                              sizeof(property_size),
                          ESP_FAIL, exit_cleanup_property_data, TAG, "Failed to send dynamic property size");
    }

    /* Send the property data. */
    ESP_GOTO_ON_FALSE(chain_comm_write_bytes(UART_NUM, property_data, property_size, &tx_checksum_calc) ==
                          property_size,
                      ESP_FAIL, exit_cleanup_property_data, TAG, "Failed to send property data");

    /* Send the checksum. */
    ESP_GOTO_ON_FALSE(chain_comm_write_bytes(UART_NUM, &tx_checksum_calc, sizeof(tx_checksum_calc), NULL) ==
                          sizeof(tx_checksum_calc),
                      ESP_FAIL, exit_cleanup_property_data, TAG, "Failed to send checksum");

    /* Send message return code. */
    ESP_GOTO_ON_FALSE(chain_comm_write_bytes(UART_NUM, &msg_rc.raw, 1, NULL) == 1, ESP_FAIL, exit_cleanup_property_data,
                      TAG, "Failed to send message return code");

    /* Receive the header and compare with the original. */
    chain_comm_msg_header_t rx_header = {0};
    ESP_GOTO_ON_FALSE(chain_comm_read_bytes(UART_NUM, &rx_header, sizeof(rx_header),
                                            RX_BYTES_TIMEOUT(sizeof(rx_header)), NULL) == sizeof(rx_header),
                      ESP_FAIL, exit_cleanup_property_data, TAG, "Failed to receive header");
    ESP_GOTO_ON_FALSE(rx_header.raw == tx_header.raw, ESP_FAIL, exit_cleanup_property_data, TAG, "Header mismatch");

    /* Receive dynamic size and compare with the original.*/
    if (chain_comm_write_attr->dynamic_property_size) {
        uint16_t rx_property_size = 0;
        ESP_GOTO_ON_FALSE(chain_comm_read_bytes(UART_NUM, &rx_property_size, sizeof(rx_property_size),
                                                RX_BYTES_TIMEOUT(sizeof(rx_property_size)),
                                                NULL) == sizeof(rx_property_size),
                          ESP_FAIL, exit_cleanup_property_data, TAG, "Failed to receive property size header");
        ESP_GOTO_ON_FALSE(rx_property_size == property_size, ESP_FAIL, exit_cleanup_property_data, TAG,
                          "Property size mismatch");
    }

    /* Receive the data and compare with the original data.*/
    rx_buff = malloc(property_size);
    ESP_GOTO_ON_FALSE(rx_buff != NULL, ESP_ERR_NO_MEM, exit_cleanup_property_data, TAG, "Memory allocation failed");
    ESP_GOTO_ON_FALSE(chain_comm_read_bytes(UART_NUM, rx_buff, property_size, RX_BYTES_TIMEOUT(property_size), NULL) ==
                          property_size,
                      ESP_FAIL, exit_cleanup_rx_buff, TAG, "Failed to receive data");
    ESP_GOTO_ON_FALSE(memcmp(property_data, rx_buff, property_size) == 0, ESP_FAIL, exit_cleanup_rx_buff, TAG,
                      "TX and RD data mismatch");

    /* Verify the checksum. */
    ESP_GOTO_ON_FALSE(chain_comm_read_bytes(UART_NUM, &rx_checksum_actual, 1, RX_BYTES_TIMEOUT(1), NULL) == 1, ESP_FAIL,
                      exit_cleanup_rx_buff, TAG, "Failed to receive checksum");
    ESP_GOTO_ON_FALSE(tx_checksum_calc == rx_checksum_actual, ESP_FAIL, exit_cleanup_rx_buff, TAG, "Checksum mismatch");

    /* Receive message return code. */
    ESP_GOTO_ON_FALSE(chain_comm_read_bytes(UART_NUM, &msg_rc.raw, 1, RX_BYTES_TIMEOUT(1), NULL) == 1, ESP_FAIL,
                      exit_cleanup_rx_buff, TAG, "Failed to receive message return code");
    ESP_GOTO_ON_FALSE(msg_rc.rc == CHAIN_COMM_RC_SUCCESS, ESP_FAIL, exit_cleanup_rx_buff, TAG,
                      "Message returned an error code.");

    /*Cleanup. */
exit_cleanup_rx_buff:
    free(rx_buff);
exit_cleanup_property_data:
    free(property_data);
    return ret;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t chain_comm_property_write_seq(display_t *display, property_id_t property_id)
{
    uint8_t *property_data = NULL;
    uint16_t property_size = 0;

    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "display is NULL");

    const char *property_name = chain_comm_property_name_by_id(property_id);
    ESP_RETURN_ON_FALSE(property_name != NULL, ESP_ERR_INVALID_ARG, TAG, "property %d invalid", property_id);

    const property_handler_t *property_handler = property_handler_get_by_id(property_id);
    ESP_RETURN_ON_FALSE(property_handler != NULL, ESP_ERR_NOT_SUPPORTED, TAG, "No handler for [%s] property.",
                        property_name);
    ESP_RETURN_ON_FALSE(property_handler->to_binary != NULL, ESP_ERR_NOT_SUPPORTED, TAG,
                        "[%s] Property is not writable.", property_name);

    module_t *module = display_module_get(display, 0);
    ESP_RETURN_ON_FALSE(module != NULL, ESP_ERR_INVALID_ARG, TAG, "No modules in display");
    ESP_LOGI(TAG, "Writing sequential property %s", property_name);

    /* Flush uart RX buffer. */
    uart_flush_input(UART_NUM);

    uint16_t display_size = display_size_get(display);
    for (uint16_t i = 0; i < display_size; i++) {
        module_t *module         = display_module_get(display, i);
        uint8_t tx_checksum_calc = 0;

        if (module_property_is_desynchronized(module, property_id)) {
            /* Property needs to be written to current module. */

            /* Initiate the message. */
            chain_comm_msg_header_t tx_header = {.action = property_writeSequential, .property = property_id};

            /* Send the header. */
            ESP_RETURN_ON_FALSE(chain_comm_write_bytes(UART_NUM, &tx_header, sizeof(tx_header), &tx_checksum_calc) ==
                                    sizeof(tx_header),
                                ESP_FAIL, TAG, "Failed to send header");

            /* Set the property data of the message. */
            const chain_comm_binary_attributes_t *chain_comm_write_attr =
                chain_comm_property_write_attributes_get(property_handler->id);

            ESP_RETURN_ON_ERROR(property_handler->to_binary(&property_data, &property_size, module), TAG,
                                "Failed to get property size");

            /* Send dynamic property size. */
            if (chain_comm_write_attr->dynamic_property_size) {
                ESP_RETURN_ON_FALSE(chain_comm_write_bytes(UART_NUM, &property_size, sizeof(property_size),
                                                           &tx_checksum_calc) == sizeof(property_size),
                                    ESP_FAIL, TAG, "Failed to send dynamic property size");
            }

            /* Send the property data. */
            ESP_RETURN_ON_FALSE(chain_comm_write_bytes(UART_NUM, property_data, property_size, &tx_checksum_calc) ==
                                    property_size,
                                ESP_FAIL, TAG, "Failed to send property data");

        } else {
            /* Property do's not needs to be written to current module. */
            /* Initiate the message. Send PROPERTY_NONE to skip the module. */
            chain_comm_msg_header_t tx_header = {.action = property_writeSequential, .property = PROPERTY_NONE};

            /* Send the header. */
            ESP_RETURN_ON_FALSE(chain_comm_write_bytes(UART_NUM, &tx_header, sizeof(tx_header), &tx_checksum_calc) ==
                                    sizeof(tx_header),
                                ESP_FAIL, TAG, "Failed to send header");
        }

        /* Send the checksum. */
        ESP_RETURN_ON_FALSE(chain_comm_write_bytes(UART_NUM, &tx_checksum_calc, sizeof(tx_checksum_calc), NULL) ==
                                sizeof(tx_checksum_calc),
                            ESP_FAIL, TAG, "Failed to send checksum");
    }

    /* Send first message return code, this triggers the modules to execute their command. */
    chain_comm_msg_return_code_t msg_rc = {.action = returnCode, .rc = CHAIN_COMM_RC_SUCCESS};
    ESP_RETURN_ON_FALSE(chain_comm_write_bytes(UART_NUM, &msg_rc.raw, 1, NULL) == 1, ESP_FAIL, TAG,
                        "Failed to send first message return code");
    /* Send second message return code, when this data returns, we know the modules have finished their command. */
    ESP_RETURN_ON_FALSE(chain_comm_write_bytes(UART_NUM, &msg_rc.raw, 1, NULL) == 1, ESP_FAIL, TAG,
                        "Failed to send second message return code");

    /* Receive first message return code. */
    ESP_RETURN_ON_FALSE(chain_comm_read_bytes(UART_NUM, &msg_rc.raw, 1, RX_BYTES_TIMEOUT(1), NULL) == 1, ESP_FAIL, TAG,
                        "Failed to receive message return code");
    ESP_RETURN_ON_FALSE(msg_rc.rc == CHAIN_COMM_RC_SUCCESS, ESP_FAIL, TAG, "Message return code 1 mismatch");

    /* Receive second message return code. */
    ESP_RETURN_ON_FALSE(chain_comm_read_bytes(UART_NUM, &msg_rc.raw, 1, RX_BYTES_TIMEOUT(1), NULL) == 1, ESP_FAIL, TAG,
                        "Failed to receive message return code");
    ESP_RETURN_ON_FALSE(msg_rc.rc == CHAIN_COMM_RC_SUCCESS, ESP_FAIL, TAG, "Message return code 2 mismatch");

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

static void chain_comm_task(void *arg)
{
    chain_comm_ctx_t *ctx = (chain_comm_ctx_t *)arg;

    while (1) {
        display_event_wait_for_desynchronized(ctx->display, portMAX_DELAY);
        ESP_LOGI(TAG, "Display desynchronized");

        /* Promote possible write seq to write all. */
        display_property_promote_write_seq_to_write_all(ctx->display);

        for (property_id_t property = PROPERTY_NONE + 1; property < PROPERTIES_MAX; property++) {
            if (display_property_is_desynchronized(ctx->display, property, PROPERTY_SYNC_METHOD_READ)) {
                esp_err_t err = chain_comm_property_read_all(ctx->display, property);
                if (err == ESP_OK || err == ESP_ERR_NOT_SUPPORTED) {
                    display_property_indicate_synchronized(ctx->display, property);
                }
            } else if (display_property_is_desynchronized(ctx->display, property, PROPERTY_SYNC_METHOD_WRITE)) {
                for (int attempt_cnt = 3; attempt_cnt > 0; attempt_cnt--) {
                    esp_err_t err = chain_comm_property_write_all(ctx->display, property);
                    if (err == ESP_OK || err == ESP_ERR_NOT_SUPPORTED) {
                        display_property_indicate_synchronized(ctx->display, property);
                        break;
                    } else {
                        ESP_LOGW(TAG, "Failed to write property, attempting %d more time", attempt_cnt - 1);
                    }
                }
            } else {
                uint16_t display_size = display_size_get(ctx->display);
                for (uint16_t i = 0; i < display_size; i++) {
                    module_t *module = display_module_get(ctx->display, i);
                    if (module_property_is_desynchronized(module, property)) {
                        esp_err_t err = chain_comm_property_write_seq(ctx->display, property);
                        if (err == ESP_ERR_NOT_SUPPORTED) {
                            module_property_indicate_synchronized(module, property);
                        }
                        break;
                    }
                }
            }
#if CHAIN_COMM_DEBUG
            vTaskDelay(pdMS_TO_TICKS(50));
#endif
        }

        ESP_LOGI(TAG, "Display synchronized");
        display_event_synchronized(ctx->display);
    }
}

//---------------------------------------------------------------------------------------------------------------------

static int chain_comm_write_bytes(uart_port_t uart_num, const void *src, size_t size, uint8_t *checksum)
{
    int s = uart_write_bytes(uart_num, src, size);
    for (size_t i = 0; checksum != NULL && i < s; i++) {
        *checksum += ((const uint8_t *)src)[i];
    }
    return s;
}

//---------------------------------------------------------------------------------------------------------------------

static int chain_comm_read_bytes(uart_port_t uart_num, void *buf, uint32_t length, TickType_t ticks_to_wait,
                                 uint8_t *checksum)
{
    int s = uart_read_bytes(uart_num, buf, length, ticks_to_wait);
    for (size_t i = 0; checksum != NULL && i < s; i++) {
        *checksum += ((const uint8_t *)buf)[i];
    }
    return s;
}

//---------------------------------------------------------------------------------------------------------------------