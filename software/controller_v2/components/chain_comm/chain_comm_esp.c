#include "chain_comm_esp.h"
#include "driver/uart.h"
#include "esp_check.h"
#include "esp_log.h"
#include "properties.h"

#include <string.h>

#define TAG "CHAIN_COMM"

#define UART_BUF_SIZE        (1024)
#define UART_NUM             UART_NUM_1
#define TX_PIN               (10)
#define RX_PIN               (9)
#define CHAIN_COMM_TASK_SIZE 6000

static void chain_comm_task(void *arg);

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

    ESP_RETURN_ON_FALSE(xTaskCreate(chain_comm_task, "chain_comm_task", CHAIN_COMM_TASK_SIZE, ctx, 10, NULL), ESP_FAIL,
                        TAG, "Failed to create chain comm task");

    return ESP_OK;
}

static void chain_comm_task(void *arg)
{
    chain_comm_ctx_t *ctx = (chain_comm_ctx_t *)arg;

    while (1) {
        display_event_wait_for_desynchronized(ctx->display, portMAX_DELAY);
        ESP_LOGI(TAG, "Display desynchronized");
        if (display_size_get(ctx->display) == 0) {
            ESP_LOGW(TAG, "No modules in model.");
            chain_comm_property_read_all(ctx->display, PROPERTY_MODULE_INFO);
        }
        display_event_synchronized(ctx->display);
    }
}

esp_err_t chain_comm_property_read_all(display_t *display, property_id_t property)
{
    ESP_LOGI(TAG, "Reading all property %s", chain_comm_property_name_get(property));
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "display is NULL");
    const property_handler_t *property_handler = property_handler_get_by_id(property);
    ESP_RETURN_ON_FALSE(property_handler != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid property");
    ESP_RETURN_ON_FALSE(property_handler->from_binary != NULL, ESP_ERR_INVALID_ARG, TAG, "Property is not readable");

    /* Initiate the message. */
    chainCommMessage_t tx_msg = {0};
    chain_comm_msg_init_read_all(&tx_msg, property);

    /* Flush uart RX buffer. */
    uart_flush_input(UART_NUM);

    /* Send the message. */
    ESP_RETURN_ON_FALSE(uart_write_bytes(UART_NUM, tx_msg.raw, tx_msg.size) == tx_msg.size, ESP_FAIL, TAG,
                        "Failed to send read all message");

    /* Receive the header. */
    chainCommMessage_t rx_msg = {0};
    ESP_RETURN_ON_FALSE(uart_read_bytes(UART_NUM, rx_msg.raw, READ_HEADER_LEN, 250 / portTICK_PERIOD_MS) ==
                            READ_HEADER_LEN,
                        ESP_FAIL, TAG, "Failed to receive read all header");
    ESP_RETURN_ON_FALSE(tx_msg.structured.header.raw == rx_msg.structured.header.raw, ESP_FAIL, TAG, "Header mismatch");

    /* Validate the size. */
    int16_t module_cnt = rx_msg.structured.data[0] + rx_msg.structured.data[1] * 0xff;

    if (display_size_get(display) != module_cnt) {
        ESP_LOGW(TAG, "The display size has changed from %d to %d, resizing display...", display_size_get(display),
                 module_cnt);
        display_resize(display, module_cnt);
    }

    /* Receive the data. */
    for (uint16_t i = 0; i < module_cnt; i++) {
        const chain_comm_binary_attributes_t *chain_comm_read_attr =
            chain_comm_property_read_attributes_get(property_handler->id);
        uint16_t property_size = chain_comm_read_attr->static_size;
        if (chain_comm_read_attr->dynamic_size) {
            ESP_LOGE(TAG, "Dynamic size not supported yet");
            continue;
        }
        if (chain_comm_read_attr->multipart) {
            ESP_LOGE(TAG, "Multipart not supported yet");
            continue;
        }

        chain_comm_msg_init(&rx_msg);
        ESP_RETURN_ON_FALSE(uart_read_bytes(UART_NUM, rx_msg.raw, property_size, 250 / portTICK_PERIOD_MS) ==
                                property_size,
                            ESP_FAIL, TAG, "Failed to receive read all data");
        module_t *module      = display_module_get(display, i);
        void *module_property = module_property_get_by_id(module, property);
        ESP_RETURN_ON_ERROR(property_handler->from_binary(module_property, rx_msg.raw, 0), TAG,
                            "Failed to convert read all data");
    }

    /* Receive the data. */
    return ESP_OK;
}

esp_err_t chain_comm_property_write_all(display_t *display, property_id_t property)
{
    ESP_RETURN_ON_FALSE(display == NULL, ESP_ERR_INVALID_ARG, TAG, "display is NULL");

    return ESP_OK;
}

esp_err_t chain_comm_property_write_seq(display_t *display, property_id_t property)
{
    ESP_RETURN_ON_FALSE(display == NULL, ESP_ERR_INVALID_ARG, TAG, "display is NULL");

    return ESP_OK;
}