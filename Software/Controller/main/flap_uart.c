#include "flap_uart.h"

static const char *TAG = "[UART]";

// static QueueHandle_t uart_rx_queue;
static QueueHandle_t uart_tx_queue;
static TaskHandle_t flap_uart_taskHandle;

// static bool enable = true;

typedef struct{
    int data_len;
    char data[512];
}uart_tx_data_t;

static void flap_uart_task(void *arg)
{
    controller_queue_data_t *controller_comm = calloc(1,sizeof(controller_queue_data_t));
    controller_comm->cmd = controller_rx_data;
    uart_tx_data_t *tx_data = NULL;
    while (1){
        if (xQueueReceive(uart_tx_queue, (void *)&tx_data, 50 / portTICK_RATE_MS)){
            char *buf = calloc(1,tx_data->data_len*5*sizeof(char)+1);
            for(int i = 0;i<tx_data->data_len;i++){
                sprintf(buf+5*i,"0x%02x ",tx_data->data[i]);
            }
            ESP_LOGI(TAG,"sending uart: %s",buf);
            free(buf);
            for(int i=0; i<tx_data->data_len;){
                i+= uart_write_bytes(UART_NUM, tx_data->data+i, 1);
                vTaskDelay(2 / portTICK_RATE_MS);
            }
            if(tx_data->data[0] & 0x80){
                if((tx_data->data[0] & 0x7F) != module_read_data){  // NOT A READ COMMAND
                    ESP_LOGI(TAG,"Expecting %d returning bytes",tx_data->data_len);
                    int rx_len = 0, cnt = 0;
                    while(rx_len < tx_data->data_len && cnt++ < 5){
                        rx_len += uart_read_bytes(UART_NUM, controller_comm->data+rx_len, tx_data->data_len, 50 / portTICK_RATE_MS);
                        if(rx_len < tx_data->data_len) ESP_LOGW(TAG,"partial data: %d",rx_len);
                    }
                    for(int i=0;i<tx_data->data_len;i++){
                        if(tx_data->data[i] != controller_comm->data[i]) ESP_LOGE(TAG,"Received data does not match transmitted data 0x%02x >> 0x%02x",tx_data->data[i], controller_comm->data[i]);
                    }


                    ESP_LOGI(TAG,"Got %d returning bytes",rx_len);
                    char *buf = calloc(1,rx_len*5*sizeof(char)+1);
                    for(int i = 0;i<rx_len;i++){
                        sprintf(buf+5*i,"0x%02x ",controller_comm->data[i]);
                    }ESP_LOGI(TAG,"got %s",buf);free(buf);
                }else{  // READ COMMAND
                    if(tx_data->data_len == 4){
                        ESP_LOGI(TAG,"Expecting 4 returning bytes");
                        int rx_len = 0, cnt = 0;
                        while(rx_len < tx_data->data_len && cnt++ < 5){
                            rx_len += uart_read_bytes(UART_NUM, controller_comm->data+rx_len, tx_data->data_len, 50 / portTICK_RATE_MS);
                        }
                        char *buf = calloc(1,rx_len*5*sizeof(char)+1);
                        for(int i = 0;i<rx_len;i++){
                            sprintf(buf+5*i,"0x%02x ",controller_comm->data[i]);
                        }ESP_LOGI(TAG,"got %s",buf);free(buf);
                        int cnt_modules = (controller_comm->data[2] << 8) + controller_comm->data[1];
                        int cnt_data = controller_comm->data[3];
                        ESP_LOGI(TAG,"Expecting %d bytes from %d modules returning data",cnt_data,cnt_modules);
                        controller_comm->total_data_len = cnt_modules * cnt_data;
                        controller_comm->data_len = cnt_data;
                        controller_comm->data_offset = 0;
                        for(int i = 0; i < cnt_modules;i++){
                            int rx_len = 0, timeout_cnt = 0;
                            while(rx_len < cnt_data && timeout_cnt++ < 17){
                                rx_len += uart_read_bytes(UART_NUM, controller_comm->data+rx_len, cnt_data-rx_len, 50 / portTICK_RATE_MS);
                            }
                            if(rx_len){
                                char *buf = calloc(1,rx_len*5*sizeof(char)+1);
                                for(int i = 0;i<rx_len;i++){
                                    sprintf(buf+5*i,"0x%02x ",controller_comm->data[i]);
                                }ESP_LOGI(TAG,"got %s",buf);free(buf);
                                controller_respons_enqueue(controller_comm);
                                controller_comm->data_offset += cnt_data;
                            }
                        }
                    }else{
                        ESP_LOGI(TAG,"Unexpected data lenght of %d for read command",tx_data->data_len);
                    }
                }
            }
            free(tx_data);
        }
            int rx_len = uart_read_bytes(UART_NUM, controller_comm->data, 120, 10 / portTICK_RATE_MS);
            if(rx_len){
                ESP_LOGI(TAG,"Received %d bytes of unexpected uart data",rx_len);
                char *buf = calloc(1,rx_len*5*sizeof(char)+1);
                for(int i = 0;i<rx_len;i++){
                    sprintf(buf+5*i,"0x%02x ",controller_comm->data[i]);
                }ESP_LOGI(TAG,"got %s",buf);free(buf);
            }
    }
}

void flap_uart_send_data(char* data, int data_len)
{
    uart_tx_data_t *tx_data = malloc(sizeof(uart_tx_data_t));
    memcpy(tx_data->data,data,data_len);
    tx_data->data_len = data_len;
    xQueueSend(uart_tx_queue,&tx_data,(portTickType)portMAX_DELAY);
}

void flap_uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = 9600,//115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    uart_tx_queue = xQueueCreate(1, sizeof(uart_tx_data_t*));
	configASSERT(uart_tx_queue);

    xTaskCreate(flap_uart_task, "flap_uart_task", 6000, NULL, 10, &flap_uart_taskHandle);
}
