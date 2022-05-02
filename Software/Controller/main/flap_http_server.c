#include "flap_http_server.h"

// #define MAX_WS_CONNECTIONS 3
static const char *TAG = "[HTTP]";

static httpd_handle_t server = NULL;

static int httpd_req_recv_blocking(httpd_req_t *req, char* data, int len)
{
    int total_recv = 0, curr_recv = 0;
    do{
        curr_recv = httpd_req_recv(req, data + total_recv, len - total_recv);
        if(curr_recv < 0){
            ESP_LOGE(TAG,"An error occured while reading data");
            return -curr_recv;
        }else if(curr_recv == 0){
            ESP_LOGI(TAG,"There is tempoarily no more data.");
            taskYIELD();
        }
        total_recv += curr_recv;
    }while(total_recv < len);
    return total_recv;
}

static esp_err_t index_page_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req,"text/html");
    httpd_resp_send(req, (const char *)index_start, index_end - index_start);
    return ESP_OK;
}
static const httpd_uri_t index_page = {
    .uri          = "/",
    .method       = HTTP_GET,
    .handler      = index_page_get_handler,
    .user_ctx     = NULL
};

typedef enum{
    update_module,
    update_controller
}update_firmware_t;

static esp_err_t firmware_handler(httpd_req_t *req)
{
    static update_firmware_t update;
    controller_queue_data_t *controller_comm = calloc(1,sizeof(controller_queue_data_t));
    int read_cnt = 0;
    int boundry_length = 0;
    while(read_cnt <= 4 || controller_comm->data[read_cnt-4] != '\r' || controller_comm->data[read_cnt-3] != '\n' || controller_comm->data[read_cnt-2] != '\r' || controller_comm->data[read_cnt-1] != '\n'){
        if (httpd_req_recv_blocking(req, controller_comm->data+read_cnt,1) <= 0){
            return ESP_FAIL;
        }
        read_cnt++;
        if(!boundry_length && controller_comm->data[read_cnt-2] == '\r' && controller_comm->data[read_cnt-1] == '\n') boundry_length = read_cnt + 4;
    }
    ESP_LOGI(TAG,"boundry len %d",boundry_length);
    ESP_LOGI(TAG,"%s",controller_comm->data);
    regex_t b;
    regmatch_t groupArray[5];
	b.re_magic=0;
	int reti;
	reti=regcomp(&b,"name=\"controller_([a-z_]*)\"",REG_EXTENDED);
	if (reti){
		printf("Can't compile re\r\n");
	}else{
		reti = regexec(&b, controller_comm->data, 5, groupArray, 0);
		if (!reti){
            for (int g = 0; g < 5; g++){
                if (groupArray[g].rm_so == (size_t)-1)break;  // No more groups
                controller_comm->data[groupArray[g].rm_eo] = 0;
                if(g==1){
                    if(!strcmp(controller_comm->data + groupArray[g].rm_so,"controller_firmware")) update = update_controller;
                    else if(!strcmp(controller_comm->data + groupArray[g].rm_so,"module_firmware")) update = update_module;
                    ESP_LOGI(TAG,"command: %s,",controller_comm->data + groupArray[g].rm_so);
                }
            }
		}
	}
	regfree(&b);
    controller_comm->total_data_len = req->content_len - read_cnt - boundry_length;
    controller_comm->data_len = CMD_COMM_BUF_LEN;
    controller_comm->data_offset = 0;

    ESP_LOGI(TAG,"total usefull data len %d, + boundry %d",controller_comm->total_data_len,req->content_len);
    ESP_LOGI(TAG,"already read %d, after data read %d",read_cnt,boundry_length);

    bzero(controller_comm->data,CMD_COMM_BUF_LEN);
    while(controller_comm->data_offset + controller_comm->data_len < controller_comm->total_data_len){
        if (httpd_req_recv_blocking(req, controller_comm->data,controller_comm->data_len) <= 0){
            return ESP_FAIL;
        }
        if(update == update_controller){
            flap_controller_firmware_update(controller_comm->data,controller_comm->data_len,controller_comm->data_offset,controller_comm->total_data_len);
        }else{
            flap_module_firmware_update(controller_comm->data,controller_comm->data_len,controller_comm->data_offset,controller_comm->total_data_len);
        }
        controller_comm->data_offset += controller_comm->data_len;
    }


    controller_comm->data_len = controller_comm->total_data_len - controller_comm->data_offset;
    if(controller_comm->data_len){
        if (httpd_req_recv_blocking(req, controller_comm->data, controller_comm->data_len) <= 0){
            return ESP_FAIL;
        }
        if(update == update_controller){
            flap_controller_firmware_update(controller_comm->data,controller_comm->data_len,controller_comm->data_offset,controller_comm->total_data_len);
        }else{
            flap_module_firmware_update(controller_comm->data,controller_comm->data_len,controller_comm->data_offset,controller_comm->total_data_len);
        }
        controller_comm->data_offset += controller_comm->data_len;
    }
    if (httpd_req_recv_blocking(req, controller_comm->data,boundry_length) <= 0){
        return ESP_FAIL;
    }
    ESP_LOGI(TAG,"%.*s",boundry_length,controller_comm->data);

    free(controller_comm);
    
    httpd_resp_set_status(req, "204 No Content");
    httpd_resp_set_type(req,"text/html");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}
static const httpd_uri_t firmware = {
    .uri          = "/firmware",
    .method       = HTTP_POST,
    .handler      = firmware_handler,
    .user_ctx     = NULL,
};

static esp_err_t style_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)style_start, style_end - style_start);
    return ESP_OK;
}
static const httpd_uri_t style = {
    .uri          = "/style.css",
    .method       = HTTP_GET,
    .handler      = style_get_handler,
    .user_ctx     = NULL};

static esp_err_t favicon_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "image/svg+xml");
    httpd_resp_send(req, (const char *)favicon_start, favicon_end - favicon_start);
    return ESP_OK;
}
static const httpd_uri_t favicon = {
    .uri          = "/favicon.svg",
    .method       = HTTP_GET,
    .handler      = favicon_get_handler,
    .user_ctx     = NULL};

void flap_init_webserver()
{
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &http_server_connect_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &http_server_disconnect_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &http_server_connect_handler, NULL));
}

httpd_handle_t flap_start_webserver(void)
{
    // httpd_handle_t server;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 20;
    // config.stack_size = 8000;
    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &index_page);
        httpd_register_uri_handler(server, &firmware);
        httpd_register_uri_handler(server, &style);
        httpd_register_uri_handler(server, &favicon);
        // httpd_register_uri_handler(server, &ws);
        add_api_endpoints(&server);
        return server;
    }
    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void stop_webserver()
{
    httpd_stop(server);
}

void http_server_disconnect_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data)
{
    if (server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver();
        server = NULL;
    }
}

void http_server_connect_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data)
{
    if (server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        server = flap_start_webserver();
    }
}
