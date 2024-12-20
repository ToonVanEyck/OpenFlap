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

#define RX_BYTES_TIMEOUT(_byte_cnt) (((_byte_cnt) * 10) / portTICK_PERIOD_MS)

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

        // if (display_size_get(ctx->display) == 0) {
        //     ESP_LOGW(TAG, "No modules in model.");
        //     chain_comm_property_read_all(ctx->display, PROPERTY_MODULE_INFO);
        // }

        for (property_id_t p = PROPERTY_NONE + 1; p < PROPERTIES_MAX; p++) {
            if (display_property_is_desynchronized(ctx->display, p, PROPERTY_SYNC_METHOD_READ)) {
                chain_comm_property_read_all(ctx->display, p);
            }
        }

        display_event_synchronized(ctx->display);
        ESP_LOGI(TAG, "Display synchronized");
    }
}

esp_err_t chain_comm_property_read_all(display_t *display, property_id_t property_id)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "display is NULL");

    const char *property_name = chain_comm_property_name_get(property_id);
    ESP_RETURN_ON_FALSE(property_name != NULL, ESP_ERR_INVALID_ARG, TAG, "property %d invalid", property_id);

    const property_handler_t *property_handler = property_handler_get_by_id(property_id);
    ESP_RETURN_ON_FALSE(property_handler != NULL, ESP_ERR_INVALID_ARG, TAG, "No handler for property: %s",
                        property_name);
    ESP_RETURN_ON_FALSE(property_handler->from_binary != NULL, ESP_ERR_INVALID_ARG, TAG, "Property is not readable: %s",
                        property_name);

    ESP_LOGI(TAG, "Reading all property %s", property_name);

    /* Initiate the message. */
    chain_comm_msg_t tx_msg = {0};
    chain_comm_msg_init_read_all(&tx_msg, property_id);

    /* Flush uart RX buffer. */
    uart_flush_input(UART_NUM);

    /* Send the message. */
    ESP_RETURN_ON_FALSE(uart_write_bytes(UART_NUM, tx_msg.raw, tx_msg.size) == tx_msg.size, ESP_FAIL, TAG,
                        "Failed to send read all message");

    /* Receive the header. */
    chain_comm_msg_t rx_msg = {0};
    ESP_RETURN_ON_FALSE(uart_read_bytes(UART_NUM, rx_msg.raw, READ_HEADER_LEN, RX_BYTES_TIMEOUT(READ_HEADER_LEN)) ==
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

        /* Get the module. */
        module_t *module = display_module_get(display, i);
        assert(module != NULL);

        /* Get property field from the module. */
        void *module_property = module_property_get_by_id(module, property_id);
        assert(module_property != NULL);

        /* Get the property read attributes.  */
        const chain_comm_binary_attributes_t *chain_comm_read_attr =
            chain_comm_property_read_attributes_get(property_handler->id);

        /* Set the page cnt and size. */
        uint8_t property_page_cnt  = chain_comm_read_attr->static_page_cnt;
        uint8_t property_page_size = chain_comm_read_attr->static_page_size;

        if (chain_comm_read_attr->dynamic_page_cnt) {
            ESP_RETURN_ON_FALSE(uart_read_bytes(UART_NUM, &property_page_cnt, 1, RX_BYTES_TIMEOUT(1)) ==
                                    property_page_size,
                                ESP_FAIL, TAG, "Failed to receive dynamic page count");
        }

        for (uint8_t page_index = 0; page_index < property_page_cnt; page_index++) {

            if (chain_comm_read_attr->dynamic_page_size) {
                ESP_RETURN_ON_FALSE(uart_read_bytes(UART_NUM, &property_page_size, 1, RX_BYTES_TIMEOUT(1)) ==
                                        property_page_size,
                                    ESP_FAIL, TAG, "Failed to receive dynamic page size");
            }

            /* Receive the data*/
            uint8_t rx_buff[CHAIN_COM_MAX_LEN] = {0};
            ESP_RETURN_ON_FALSE(uart_read_bytes(UART_NUM, rx_buff, property_page_size,
                                                RX_BYTES_TIMEOUT(property_page_size)) == property_page_size,
                                ESP_FAIL, TAG, "Failed to receive read all data");

            ESP_RETURN_ON_ERROR(property_handler->from_binary(module_property, rx_buff, page_index), TAG,
                                "Failed to convert read all data");
        }
    }

    /* Indicate that the property has been synchronized. */
    display_property_indicate_synchronized(display, property_id);

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