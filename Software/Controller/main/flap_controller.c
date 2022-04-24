#include "flap_controller.h"


static const char *TAG = "[FLAP]";

static QueueHandle_t controller_command_queue;
static QueueHandle_t controller_respons_queue;
static TaskHandle_t flap_taskHandle;

static void flap_controller_task(void *pvParameters)
{
    controller_queue_data_t *controller_comm = NULL;
    flap_ctx_t flap_ctx;
    bzero(&flap_ctx, sizeof(flap_ctx_t));
    
    while (1){
        if (xQueueReceive(controller_command_queue, &controller_comm, 500/portTICK_PERIOD_MS)){
                switch(controller_comm->cmd){
                    case controller_goto_app:
                        flap_module_goto_app(&flap_ctx);
                        break;
                    case controller_controller_firmware:
                        flap_controller_firmware_update(controller_comm->data,controller_comm->data_len,controller_comm->data_offset,controller_comm->total_data_len);
                        break;
                    case controller_module_firmware:
                        flap_module_firmware_update(controller_comm->data,controller_comm->data_len,controller_comm->data_offset,controller_comm->total_data_len);
                        break;
                    case controller_get_dimensions:
                        flap_module_get_dimensions(&flap_ctx);
                        break;
                    case controller_set_message:
                        flap_module_set_message(&flap_ctx, controller_comm->data);
                        break;
                    case controller_get_message:
                        flap_module_get_message(&flap_ctx);
                        break;
                    case controller_set_charset:
                        flap_module_set_charset(&flap_ctx, controller_comm->data);
                        break;
                    case controller_get_charset:
                        flap_module_get_charset(&flap_ctx);
                        break;
                    case controller_set_offset:
                        flap_module_set_offset(&flap_ctx, controller_comm->data);
                        break;   
                    case controller_set_AP:
                        flap_controller_set_AP(&flap_ctx, controller_comm->data);
                        break;
                    case controller_set_STA:
                        flap_controller_set_STA(&flap_ctx, controller_comm->data);
                        break;
                    case controller_reboot:
                        esp_restart();
                        break;
                    default :
                        break;
                }
            free(controller_comm);
        }
    }
}

void controller_command_enqueue(controller_queue_data_t *controller_comm)
{
    controller_queue_data_t *controller_comm_cpy = malloc(sizeof(controller_queue_data_t));
    memcpy(controller_comm_cpy,controller_comm,sizeof(controller_queue_data_t));
    if(controller_command_queue){
        xQueueSend(controller_command_queue,&controller_comm_cpy,(portTickType)portMAX_DELAY);
    }else{
        ESP_LOGE(TAG,"Queue controller_command_queue is NULL");
    }
}

void controller_respons_enqueue(controller_queue_data_t *controller_comm)
{
    ESP_LOGI(TAG,"queue command");
    controller_queue_data_t *controller_comm_cpy = malloc(sizeof(controller_queue_data_t));
    memcpy(controller_comm_cpy,controller_comm,sizeof(controller_queue_data_t));
    xQueueSend(controller_respons_queue,&controller_comm_cpy,(portTickType)portMAX_DELAY);
}

void flap_controller_init()
{	
	controller_command_queue = xQueueCreate(1, sizeof(controller_queue_data_t*));
    configASSERT(controller_command_queue);
    controller_respons_queue = xQueueCreate(1, sizeof(controller_queue_data_t*));
	configASSERT(controller_respons_queue);
    xTaskCreate(flap_controller_task, "flap_controller", 10000, NULL, 8, &flap_taskHandle);
    configASSERT(flap_taskHandle);
}

void flap_module_get_dimensions(flap_ctx_t *flap_ctx)
{
    int len = 0;
    int rcv_complete = 0;
    controller_queue_data_t *uart_response = NULL;

    char msg_buf[257]={0};
    len = module_get_config_msg(msg_buf,1); // Send the read_config command.
    flap_uart_send_data(msg_buf,len);
    len = module_read_data_msg(msg_buf,1);  // Send a read command.
    flap_uart_send_data(msg_buf,len); 
    
    int number_of_modules = 0;
    flap_ctx->cols = 0;
    flap_ctx->rows = 0;

    do{
        if(xQueueReceive(controller_respons_queue, &uart_response, 2000/portTICK_PERIOD_MS)){
            number_of_modules = uart_response->total_data_len / uart_response->data_len;
            if(uart_response->data[0]) flap_ctx->cols++;
            ESP_LOGI(TAG,"%d %d",uart_response->data[0], flap_ctx->cols);
            rcv_complete = uart_response->data_offset + uart_response->data_len >= uart_response->total_data_len;
            free(uart_response);
        }else{
            rcv_complete = -1;
        }
    }while(!rcv_complete);
    if(rcv_complete > 0 && flap_ctx->cols){
        flap_ctx->rows = number_of_modules/flap_ctx->cols;   
        char *buf = NULL;
        asprintf(&buf,WEB_SET_DIMENSIONS_TEMPLATE(flap_ctx->cols,flap_ctx->rows));
        trigger_async_send(buf);
        free(buf);
    }
}

void flap_module_get_message(flap_ctx_t *flap_ctx)
{
    int len = 0;
    int rcv_complete = 0;
    controller_queue_data_t *uart_response = NULL;

    char msg_buf[257]={0};
    len = module_get_char_msg(msg_buf,1);
    flap_uart_send_data(msg_buf,len);  
    len = module_read_data_msg(msg_buf,4);
    flap_uart_send_data(msg_buf,len);    

    char* message = calloc(1, 4 * flap_ctx->cols * flap_ctx->rows);
    if(message == NULL){
        ESP_LOGI(TAG,"Failed to allocate memory for command");
        return;
    }
    do{
        if(xQueueReceive(controller_respons_queue, &uart_response, 2000/portTICK_PERIOD_MS)){
            sprintf(message + strlen(message),"%s",uart_response->data);
            rcv_complete = uart_response->data_offset + uart_response->data_len >= uart_response->total_data_len;
            free(uart_response);
        }else{
            rcv_complete = 1;
        }
    }while(!rcv_complete);
    if(rcv_complete > 0){
        char *buf = NULL;
        asprintf(&buf,WEB_SET_CHAR_TEMPLATE(message));
        trigger_async_send(buf);
        free(buf);
    } 
    free(message);
}

void flap_module_set_message(flap_ctx_t *flap_ctx, char *message)
{
    int len = 5 * flap_ctx->cols * flap_ctx->rows;
    int t = 0;
    char *msg_buf = calloc(1, len);
    if(msg_buf == NULL){
        ESP_LOGI(TAG,"Failed to allocate memory for command");
        return;
    }
    for(int i = 0; i<strlen(message); i++){
        char buf[4] = {0};
        int utf8_len = 1;
        buf[0] = message[i] ;        
        if(buf[0] == 'c' )          buf[utf8_len++] = message[++i];                                                                    
        if((buf[0] & 0xC0) == 0xC0) buf[utf8_len++] = message[++i];                                                                        
        if((buf[0] & 0xE0) == 0xE0) buf[utf8_len++] = message[++i];                                                                        
        if((buf[0] & 0xF0) == 0xF0) buf[utf8_len++] = message[++i];                                                                        
        msg_buf[t] = module_set_char;
        memcpy(msg_buf+t+1,buf,4);
        t+= (5*flap_ctx->rows);
        if(t >= len) t -= (len-5);
    }
    flap_uart_send_data(msg_buf,len);
    free(msg_buf);
}

void flap_module_goto_app(flap_ctx_t *flap_ctx)
{
    int len = 0;

    char msg_buf[257]={0};
    len = module_goto_app_msg(msg_buf,1);
    flap_uart_send_data(msg_buf,len); 
    vTaskDelay(250 / portTICK_PERIOD_MS);
}

void flap_module_get_charset(flap_ctx_t *flap_ctx)
{
    int len = 0;
    int rcv_complete = 0;
    controller_queue_data_t *uart_response = NULL;

    char msg_buf[257]={0};
    len = module_get_charset_msg(msg_buf,1);
    flap_uart_send_data(msg_buf,len);  
    len = module_read_data_msg(msg_buf,4*48);
    flap_uart_send_data(msg_buf,len);    

    do{
        if(xQueueReceive(controller_respons_queue, &uart_response, 2000/portTICK_PERIOD_MS)){
            char *buf = NULL;
            asprintf(&buf,WEB_SET_CHARSET_TEMPLATE(uart_response->data_offset / uart_response->data_len,uart_response->data));
            trigger_async_send(buf);
            free(buf);
            rcv_complete = uart_response->data_offset + uart_response->data_len >= uart_response->total_data_len;
            free(uart_response);
        }else{
            rcv_complete = 1;
        }
    }while(!rcv_complete);
}

void flap_module_set_charset(flap_ctx_t *flap_ctx, char *charset)
{
    uint16_t flap_id = ((charset[0] << 8) + charset[1]);
    int i = 0;
    char *buf = malloc(1 + 4*48 + flap_id);
    while(i < flap_id){
        buf[i++] = module_do_nothing;
    }
    buf[i++] = module_set_charset;
    memcpy(buf+i,charset+2,4*48);
    // charset[1] = module_set_charset;
    // flap_uart_send_data(charset+1,1 + 4*48);
    flap_uart_send_data(buf, (1 + 4*48 + flap_id));
    free(buf);
}

void flap_module_set_offset(flap_ctx_t *flap_ctx, char *offset)
{
    for(int i = 0; i < flap_ctx->cols * flap_ctx->rows; i++){
        char msg_buf[257]={0};
        int len = module_set_offset_msg(msg_buf,0,offset[i]);
        flap_uart_send_data(msg_buf,len);
    }
}

void flap_controller_set_AP(flap_ctx_t *flap_ctx, char *data)
{
    cJSON * json = cJSON_Parse(data);
    cJSON * ssid = cJSON_GetObjectItemCaseSensitive(json, "ssid");
    cJSON * pwd = cJSON_GetObjectItemCaseSensitive(json, "password");
    if (cJSON_IsString(ssid)){
        flap_nvs_set_string("AP_ssid",ssid->valuestring);
    }
    if (cJSON_IsString(pwd)){
        flap_nvs_set_string("AP_pwd",pwd->valuestring);
    }
}

void flap_controller_set_STA(flap_ctx_t *flap_ctx, char *data)
{
    cJSON * json = cJSON_Parse(data);
    cJSON * ssid = cJSON_GetObjectItemCaseSensitive(json, "ssid");
    cJSON * pwd = cJSON_GetObjectItemCaseSensitive(json, "password");
    if (cJSON_IsString(ssid)){
        flap_nvs_set_string("STA_ssid",ssid->valuestring);
    }
    if (cJSON_IsString(pwd)){
        flap_nvs_set_string("STA_pwd",pwd->valuestring);
    }
}