#include "flap_api_v1.h"

static const char *TAG = "[API]";

static QueueHandle_t controller_respons_queue;

esp_err_t modules_get_dimensions(int* width, int* height)
{
    int rcv_complete = 0;
    controller_queue_data_t *uart_response = NULL;

    char cmd_uart_buf[4]={0};
    cmd_uart_buf[0] = EXTEND + module_get_config;
    flap_uart_send_data(cmd_uart_buf,1); 
    cmd_uart_buf[0] = EXTEND + module_read_data;
    cmd_uart_buf[3] = 1;
    flap_uart_send_data(cmd_uart_buf,4); 

    int number_of_modules = 0;
    *width=0;
    *height=0;
    do{
        if(xQueueReceive(controller_respons_queue, &uart_response, 2000/portTICK_PERIOD_MS)){
            number_of_modules = uart_response->total_data_len / uart_response->data_len;
            if(uart_response->data[0]) (*width)++;
            ESP_LOGI(TAG,"%d %d",uart_response->data[0], *width);
            rcv_complete = uart_response->data_offset + uart_response->data_len >= uart_response->total_data_len;
            free(uart_response);
        }else{
            rcv_complete = -1;
        }
    }while(!rcv_complete);
    if(!(rcv_complete > 0 && *width)){
        return ESP_FAIL;
    }
    *height = number_of_modules/(*width); 
    return ESP_OK;
}

void controller_respons_enqueue(controller_queue_data_t *controller_comm)
{
    ESP_LOGI(TAG,"queue command");
    controller_queue_data_t *controller_comm_cpy = malloc(sizeof(controller_queue_data_t));
    memcpy(controller_comm_cpy,controller_comm,sizeof(controller_queue_data_t));
    xQueueSend(controller_respons_queue,&controller_comm_cpy,(portTickType)portMAX_DELAY);
}

void json_get_dimensions(cJSON *json, int* width, int* height)
{
    *width = 0; *height = 0;
    cJSON * j_width = cJSON_GetObjectItemCaseSensitive(json, "width");
    cJSON * j_height = cJSON_GetObjectItemCaseSensitive(json, "height");
    if(cJSON_IsNumber(j_width)) *width = j_width->valueint;
    if(cJSON_IsNumber(j_height)) *height = j_height->valueint;
}

static esp_err_t api_v1_enable_post_handler(httpd_req_t *req)
{
    LARGE_REQUEST_GUARD(req);
    char *data = calloc(1,CMD_COMM_BUF_LEN);
    if (httpd_req_recv(req, data, req->content_len) <= 0){
        free(data);
        return ESP_FAIL;
    } 
    data[req->content_len]=0;
    ESP_LOGI(TAG,"%s",data);

    cJSON * json = cJSON_Parse(data);
    cJSON * enable = cJSON_GetObjectItemCaseSensitive(json, "enable");
    
    if(!cJSON_IsBool(enable)){
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, NULL, 0);
        free(data);
        return ESP_OK;
    }

    gpio_set_level(FLAP_ENABLE, enable->valueint);
    if(enable->valueint){
        vTaskDelay(50 / portTICK_PERIOD_MS);
        char cmd_uart_buf[1]={0};
        cmd_uart_buf[0] = EXTEND + module_goto_app;
        flap_uart_send_data(cmd_uart_buf,1); 
    }
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_send(req, NULL, 0);
    free(data);
    return ESP_OK;
}
static const httpd_uri_t enable_post = {
    .uri          = "/api/v1/enable",
    .method       = HTTP_POST,
    .handler      = api_v1_enable_post_handler,
    .user_ctx     = NULL
};

static esp_err_t api_v1_reboot_post_handler(httpd_req_t *req)
{
    LARGE_REQUEST_GUARD(req);
    char *data = calloc(1,CMD_COMM_BUF_LEN);
    if (httpd_req_recv(req, data, req->content_len) <= 0){
        free(data);
        return ESP_FAIL;
    } 
    data[req->content_len]=0;
    ESP_LOGI(TAG,"%s",data);

    cJSON * json = cJSON_Parse(data);
    cJSON * reboot = cJSON_GetObjectItemCaseSensitive(json, "reboot");
    
    if(!cJSON_IsBool(reboot)){
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, NULL, 0);
        free(data);
        return ESP_OK;
    }

    if(reboot->valueint){
        esp_restart();
    }

    httpd_resp_set_status(req, "200 OK");
    httpd_resp_send(req, NULL, 0);
    free(data);
    return ESP_OK;
}
static const httpd_uri_t reboot_post = {
    .uri          = "/api/v1/reboot",
    .method       = HTTP_POST,
    .handler      = api_v1_reboot_post_handler,
    .user_ctx     = NULL
};

static esp_err_t api_v1_offset_post_handler(httpd_req_t *req)
{
    LARGE_REQUEST_GUARD(req);
    char *data = calloc(1,CMD_COMM_BUF_LEN);
    if (httpd_req_recv(req, data, req->content_len) <= 0){
        free(data);
        return ESP_FAIL;
    } 
    data[req->content_len]=0;
    ESP_LOGI(TAG,"%s",data);

    cJSON * json = cJSON_Parse(data);
    cJSON * dimensions = cJSON_GetObjectItemCaseSensitive(json, "dimensions");
    int width,height;
    json_get_dimensions(dimensions,&width,&height);
    cJSON * offset = cJSON_GetObjectItemCaseSensitive(json, "offset");
    
    ESP_LOGI(TAG,"%d %d %d",width,height,cJSON_GetArraySize(offset));

    if(!(cJSON_IsArray(offset) && cJSON_GetArraySize(offset) == (width * height))){
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, NULL, 0);
        free(data);
        return ESP_OK;
    }

    cJSON *offset_i = NULL;
    cJSON_ArrayForEach(offset_i, offset){
        char cmd_uart_buf[2]={0};
        cmd_uart_buf[0] = module_set_offset;
        cmd_uart_buf[0] = offset_i->valueint;
        flap_uart_send_data(cmd_uart_buf,2);
    }

    httpd_resp_set_status(req, "200 OK");
    httpd_resp_send(req, NULL, 0);
    free(data);
    return ESP_OK;
}
static const httpd_uri_t offset_post = {
    .uri          = "/api/v1/offset",
    .method       = HTTP_POST,
    .handler      = api_v1_offset_post_handler,
    .user_ctx     = NULL
};

static esp_err_t api_v1_charset_post_handler(httpd_req_t *req)
{
    LARGE_REQUEST_GUARD(req);
    char *data = calloc(1,CMD_COMM_BUF_LEN);
    if (httpd_req_recv(req, data, req->content_len) <= 0){
        free(data);
        return ESP_FAIL;
    } 
    data[req->content_len]=0;
    ESP_LOGI(TAG,"%s",data);

    cJSON * json = cJSON_Parse(data);
    cJSON * j_flap_id = cJSON_GetObjectItemCaseSensitive(json, "flap_id");
    cJSON * j_charset = cJSON_GetObjectItemCaseSensitive(json, "charset");

    int flap_id = -1;
    if(cJSON_IsNumber(j_flap_id)) flap_id = j_flap_id->valueint;

    if(!(cJSON_IsArray(j_charset) && cJSON_GetArraySize(j_charset) == 48 && flap_id >= 0)){
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, NULL, 0);
        free(data);
        return ESP_OK;
    }

    char *buf = calloc(1,1 + 4*48 + flap_id);
    if(!buf){
        ESP_LOGE(TAG,"CANT ALLOCATE MEMORY");
    }
    int i = 0;
    while(i < flap_id){
        buf[i++] = module_do_nothing;
    }
    buf[i++] = module_set_charset;
    cJSON *char_it = NULL;
    cJSON_ArrayForEach(char_it, j_charset){
        strncpy(buf+i,char_it->valuestring,4);
        i+=4;
    }
    flap_uart_send_data(buf, (1 + 4*48 + flap_id));
    free(buf);

    httpd_resp_set_status(req, "200 OK");
    httpd_resp_send(req, NULL, 0);
    free(data);
    return ESP_OK;
}
static const httpd_uri_t charset_post = {
    .uri          = "/api/v1/charset",
    .method       = HTTP_POST,
    .handler      = api_v1_charset_post_handler,
    .user_ctx     = NULL
};

static esp_err_t api_v1_charset_get_handler(httpd_req_t *req)
{
    int rcv_complete = 0;
    controller_queue_data_t *uart_response = NULL;

    char cmd_uart_buf[4]={0};
    cmd_uart_buf[0] = EXTEND + module_get_charset;
    flap_uart_send_data(cmd_uart_buf,1);   
    cmd_uart_buf[0] = EXTEND + module_read_data;
    cmd_uart_buf[3] = 4*48;
    flap_uart_send_data(cmd_uart_buf,4);   

    cJSON *resp = cJSON_CreateArray();

    do{
        if(xQueueReceive(controller_respons_queue, &uart_response, 2000/portTICK_PERIOD_MS)){
            cJSON *charset_res = cJSON_CreateObject();
            cJSON_AddNumberToObject(charset_res, "flap_id", uart_response->data_offset / uart_response->data_len);
            cJSON *charset = cJSON_AddArrayToObject(charset_res, "charset");
            for(int i = 0;i<48;i++){
                cJSON *j_char = cJSON_CreateString(uart_response->data+(4*i));
                cJSON_AddItemToArray(charset,j_char);
            }
            cJSON_AddItemToArray(resp, charset_res);
            free(uart_response);
        }else{
            rcv_complete = 1;
        }
    }while(!rcv_complete);

    if(!rcv_complete){
        httpd_resp_set_status(req, "504 Gateway Timeout");
        httpd_resp_send(req, NULL, 0);
        cJSON_Delete(resp);
        return ESP_OK;
    } 
    

    char *buf = cJSON_PrintUnformatted(resp);
    ESP_LOGI(TAG,"%s",buf);
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, (const char *)buf, strlen(buf));
    cJSON_free(buf);
    cJSON_Delete(resp);

    return ESP_OK;
}
static const httpd_uri_t charset_get = {
    .uri          = "/api/v1/charset",
    .method       = HTTP_GET,
    .handler      = api_v1_charset_get_handler,
    .user_ctx     = NULL
};

// static const httpd_uri_t revolutions_get = {
//     .uri          = "/api/v1/revolutions",
//     .method       = HTTP_GET,
//     .handler      = api_v1_handler,
//     .user_ctx     = NULL
// };

static esp_err_t api_v1_dimensions_get_handler(httpd_req_t *req)
{
    int width=0,height=0;
    modules_get_dimensions(&width,&height);

    char *buf = NULL;
    asprintf(&buf,"{\"dimensions\":{\"width\":%d,\"height\":%d}}",width,height);
    ESP_LOGI(TAG,"%s",buf);
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, (const char *)buf, strlen(buf));
    free(buf);

    return ESP_OK;
}
static const httpd_uri_t dimensions_get = {
    .uri          = "/api/v1/dimensions",
    .method       = HTTP_GET,
    .handler      = api_v1_dimensions_get_handler,
    .user_ctx     = NULL
};

static esp_err_t api_v1_message_post_handler(httpd_req_t *req)
{
    LARGE_REQUEST_GUARD(req);
    char *data = calloc(1,CMD_COMM_BUF_LEN);
    if (httpd_req_recv(req, data, req->content_len) <= 0){
        free(data);
        return ESP_FAIL;
    } 
    data[req->content_len]=0;
    ESP_LOGI(TAG,"%s",data);

    cJSON * json = cJSON_Parse(data);
    cJSON * dimensions = cJSON_GetObjectItemCaseSensitive(json, "dimensions");
    int width,height;
    json_get_dimensions(dimensions,&width,&height);
    cJSON * message = cJSON_GetObjectItemCaseSensitive(json, "message");
    
    if(!(cJSON_IsString(message) && strlen(message->valuestring) == (width * height))){
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, NULL, 0);
        free(data);
        return ESP_OK;
    }

    int len = 5 * width * height;
    int t = 0;
    char *cmd_uart_buf = calloc(1, len);
    if(cmd_uart_buf == NULL){
        ESP_LOGI(TAG,"Failed to allocate memory for command");
        return ESP_FAIL;
    }
    for(int i = 0; i<strlen(message->valuestring); i++){
        char buf[4] = {0};
        int utf8_len = 1;
        buf[0] = message->valuestring[i] ;        
        if(buf[0] == 'c' )          buf[utf8_len++] = message->valuestring[++i];                                                                    
        if((buf[0] & 0xC0) == 0xC0) buf[utf8_len++] = message->valuestring[++i];                                                                        
        if((buf[0] & 0xE0) == 0xE0) buf[utf8_len++] = message->valuestring[++i];                                                                        
        if((buf[0] & 0xF0) == 0xF0) buf[utf8_len++] = message->valuestring[++i];                                                                        
        cmd_uart_buf[t] = module_set_char;
        memcpy(cmd_uart_buf+t+1,buf,4);
        t+= (5*height);
        if(t >= len) t -= (len-5);
    }
    flap_uart_send_data(cmd_uart_buf,len);
    free(cmd_uart_buf);

    httpd_resp_set_status(req, "200 OK");
    httpd_resp_send(req, NULL, 0);
    free(data);
    return ESP_OK;
}
static const httpd_uri_t message_post = {
    .uri          = "/api/v1/message",
    .method       = HTTP_POST,
    .handler      = api_v1_message_post_handler,
    .user_ctx     = NULL
};

static esp_err_t api_v1_message_get_handler(httpd_req_t *req)
{
    int rcv_complete = 0;
    controller_queue_data_t *uart_response = NULL;

    int width=0,height=0;
    if(modules_get_dimensions(&width,&height) == ESP_FAIL){
        httpd_resp_set_status(req, "504 Gateway Timeout");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }

    char cmd_uart_buf[4]={0};
    cmd_uart_buf[0] = EXTEND + module_get_char;
    flap_uart_send_data(cmd_uart_buf,1);
    cmd_uart_buf[0] = EXTEND + module_read_data;
    cmd_uart_buf[3] = 4;
    flap_uart_send_data(cmd_uart_buf,4);
  

    char* message = calloc(1, 4 * width * height);
    if(message == NULL){
        ESP_LOGI(TAG,"Failed to allocate memory for command");
        return ESP_FAIL;
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
    if(!rcv_complete){
        httpd_resp_set_status(req, "504 Gateway Timeout");
        httpd_resp_send(req, NULL, 0);
        free(message);
        return ESP_OK;
    } 
    

    char *buf = NULL;
    asprintf(&buf,"{\"dimensions\":{\"width\":%d,\"height\":%d},\"message\":\"%s\"}",width,height,message);
    ESP_LOGI(TAG,"%s",buf);
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, (const char *)buf, strlen(buf));
    free(buf);

    free(message);
    return ESP_OK;
}
static const httpd_uri_t message_get = {
    .uri          = "/api/v1/message",
    .method       = HTTP_GET,
    .handler      = api_v1_message_get_handler,
    .user_ctx     = NULL
};

static esp_err_t api_v1_wifiap_post_handler(httpd_req_t *req)
{
    LARGE_REQUEST_GUARD(req);
    char *data = calloc(1,CMD_COMM_BUF_LEN);
    if (httpd_req_recv(req, data, req->content_len) <= 0){
        free(data);
        return ESP_FAIL;
    } 
    data[req->content_len]=0;
    ESP_LOGI(TAG,"%s",data);

    cJSON * json = cJSON_Parse(data);
    cJSON * ssid = cJSON_GetObjectItemCaseSensitive(json, "ssid");
    cJSON * pwd = cJSON_GetObjectItemCaseSensitive(json, "password");
    
    if(!(cJSON_IsString(ssid) && cJSON_IsString(pwd))){
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, NULL, 0);
        free(data);
        return ESP_OK;
    }

    flap_nvs_set_string("AP_ssid",ssid->valuestring);
    flap_nvs_set_string("AP_pwd",pwd->valuestring);

    httpd_resp_set_status(req, "200 OK");
    httpd_resp_send(req, NULL, 0);
    free(data);
    return ESP_OK;
}
static const httpd_uri_t wifi_ap_post = {
    .uri          = "/api/v1/wifi_ap",
    .method       = HTTP_POST,
    .handler      = api_v1_wifiap_post_handler,
    .user_ctx     = NULL
};

static esp_err_t api_v1_wifista_post_handler(httpd_req_t *req)
{
    LARGE_REQUEST_GUARD(req);
    char *data = calloc(1,CMD_COMM_BUF_LEN);
    if (httpd_req_recv(req, data, req->content_len) <= 0){
        free(data);
        return ESP_FAIL;
    } 
    data[req->content_len]=0;
    ESP_LOGI(TAG,"%s",data);

    cJSON * json = cJSON_Parse(data);
    cJSON * ssid = cJSON_GetObjectItemCaseSensitive(json, "ssid");
    cJSON * pwd = cJSON_GetObjectItemCaseSensitive(json, "password");
    
    if(!(cJSON_IsString(ssid) && cJSON_IsString(pwd))){
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, NULL, 0);
        free(data);
        return ESP_OK;
    }

    flap_nvs_set_string("STA_ssid",ssid->valuestring);
    flap_nvs_set_string("STA_pwd",pwd->valuestring);

    httpd_resp_set_status(req, "200 OK");
    httpd_resp_send(req, NULL, 0);
    free(data);
    return ESP_OK;
}
static const httpd_uri_t wifi_sta_post = {
    .uri          = "/api/v1/wifi_sta",
    .method       = HTTP_POST,
    .handler      = api_v1_wifista_post_handler,
    .user_ctx     = NULL
};

void add_api_endpoints(httpd_handle_t *server)
{
    controller_respons_queue = xQueueCreate(1, sizeof(controller_queue_data_t*));
	configASSERT(controller_respons_queue);
    httpd_register_uri_handler(*server, &enable_post);
    httpd_register_uri_handler(*server, &reboot_post);
    httpd_register_uri_handler(*server, &offset_post);
    httpd_register_uri_handler(*server, &charset_post);
    httpd_register_uri_handler(*server, &charset_get);
    // httpd_register_uri_handler(*server, &revolutions_get);
    httpd_register_uri_handler(*server, &dimensions_get);
    httpd_register_uri_handler(*server, &message_post);
    httpd_register_uri_handler(*server, &message_get);
    // httpd_register_uri_handler(*server, &goto_app_post);
    // httpd_register_uri_handler(*server, &goto_btl_post);
    httpd_register_uri_handler(*server, &wifi_ap_post);
    httpd_register_uri_handler(*server, &wifi_sta_post);
}