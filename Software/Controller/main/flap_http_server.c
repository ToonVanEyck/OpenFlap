#include "flap_http_server.h"

#define MAX_WS_CONNECTIONS 3
static const char *TAG = "[HTTP]";

static httpd_handle_t server = NULL;

struct async_resp_arg {
  httpd_handle_t hd;
  int fd;
};

static void ws_async_send(void *arg)
{
    char* data = arg;
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t*)data;
    ws_pkt.len = strlen(data);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    ws_pkt.final = true;
    
    int ws_fd[MAX_WS_CONNECTIONS] = {0};
    size_t num_clients = MAX_WS_CONNECTIONS;
    if(httpd_get_client_list(server,&num_clients,ws_fd) == ESP_OK){
        ESP_LOGI(TAG,"sending \"%s\" to %d clients",data,num_clients);
        for(int i = 0;i < num_clients;i++){
            if (httpd_ws_get_fd_info(server, ws_fd[i]) == HTTPD_WS_CLIENT_WEBSOCKET) {
                if(ws_fd[i]>0){
                    ESP_LOGI(TAG,"active client: %d",ws_fd[i]);
                    if(httpd_ws_send_frame_async(server, ws_fd[i], &ws_pkt) != ESP_OK){
                        ws_fd[i]=0;
                    }
                }
            }
        }
    }
}

esp_err_t trigger_async_send(char *json_data)
{
    if(json_data == NULL) return ESP_FAIL;
    static char buf[2048] = {0};
    strcpy(buf,json_data);
    return httpd_queue_work(server, ws_async_send, buf);
}

static esp_err_t index_page_get_handler(httpd_req_t *req)
{
    controller_queue_data_t *controller_comm = calloc(1,sizeof(controller_queue_data_t));
    controller_comm->cmd = controller_goto_app;
    httpd_resp_set_type(req,"text/html");
    httpd_resp_send(req, (const char *)index_start, index_end - index_start);
    controller_command_enqueue(controller_comm);
    free(controller_comm);
    return ESP_OK;
}

static esp_err_t style_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)style_start, style_end - style_start);
    return ESP_OK;
}

static esp_err_t favicon_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "image/svg+xml");
    httpd_resp_send(req, (const char *)favicon_start, favicon_end - favicon_start);
    return ESP_OK;
}

static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");
        return ESP_OK;
    }
    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    ESP_LOGI(TAG, "frame len is %d", ws_pkt.len);
    if (ws_pkt.len) {
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
        ESP_LOGI(TAG, "Got packet with message: %s", ws_pkt.payload);
    }

    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT && ws_pkt.len < 2048) {
        controller_queue_data_t *controller_comm = calloc(1,sizeof(controller_queue_data_t));
        cJSON *ws_cmd = cJSON_Parse((char*)ws_pkt.payload);
        if(ws_cmd != NULL){
            cJSON * command = cJSON_GetObjectItemCaseSensitive(ws_cmd, "command");
            cJSON * data = cJSON_GetObjectItemCaseSensitive(ws_cmd, "data");
            cJSON * flap_id = cJSON_GetObjectItemCaseSensitive(ws_cmd, "flap_id");
            if (cJSON_IsString(command) && (command->valuestring != NULL)){
                if(!strcmp(command->valuestring,"controller_get_dimensions")){
                    controller_comm->cmd = controller_get_dimensions;
                }else if(!strcmp(command->valuestring,"controller_set_message")){
                    controller_comm->cmd = controller_set_message;
                }else if(!strcmp(command->valuestring,"controller_get_message")){
                    controller_comm->cmd = controller_get_message;
                }else if(!strcmp(command->valuestring,"controller_set_charset")){
                    controller_comm->cmd = controller_set_charset;
                    if(cJSON_IsArray(data)){
                        cJSON *char_it = NULL;
                        int i = 2;
                        if(cJSON_IsNumber(flap_id)){
                            controller_comm->data[0] = (flap_id->valueint >> 8) & 0xFF;
                            controller_comm->data[1] = (flap_id->valueint >> 0) & 0xFF;
                        }
                        cJSON_ArrayForEach(char_it, data){
                            strncpy(controller_comm->data+i,char_it->valuestring,4);
                            i+=4;
                        }
                        controller_comm->data_len = i;
                        controller_comm->total_data_len = controller_comm->data_len;
                    }
                }else if(!strcmp(command->valuestring,"controller_set_offset")){
                    controller_comm->cmd = controller_set_offset;
                    if(cJSON_IsArray(data)){
                        cJSON *char_it = NULL;
                        int i = 0;
                        cJSON_ArrayForEach(char_it, data){
                            controller_comm->data[i++] = char_it->valueint;
                        }
                        controller_comm->data_len = i;
                        controller_comm->total_data_len = controller_comm->data_len;
                    }
                }else if(!strcmp(command->valuestring,"controller_get_charset")){
                    controller_comm->cmd = controller_get_charset;
                }
            }
            if (cJSON_IsString(data) && (data->valuestring != NULL)){
                controller_comm->data_len = strlen(data->valuestring);
                controller_comm->total_data_len = controller_comm->data_len;
                strcpy(controller_comm->data,data->valuestring);
            }
            controller_command_enqueue(controller_comm);
            ESP_LOGI(TAG,"%s",command->valuestring);
        }
        cJSON_Delete(ws_cmd);
    }

    free(buf);
    return ret;
}

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

static esp_err_t post_handler(httpd_req_t *req)
{
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
                    if(!strcmp(controller_comm->data + groupArray[g].rm_so,"controller_firmware")) controller_comm->cmd = controller_controller_firmware;
                    else if(!strcmp(controller_comm->data + groupArray[g].rm_so,"module_firmware")) controller_comm->cmd = controller_module_firmware;
                    ESP_LOGI(TAG,"command[%d]: %s,",controller_comm->cmd,controller_comm->data + groupArray[g].rm_so);
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
        controller_command_enqueue(controller_comm);
        controller_comm->data_offset += controller_comm->data_len;
    }


    controller_comm->data_len = controller_comm->total_data_len - controller_comm->data_offset;
    if(controller_comm->data_len){
        if (httpd_req_recv_blocking(req, controller_comm->data, controller_comm->data_len) <= 0){
            return ESP_FAIL;
        }
        controller_command_enqueue(controller_comm);
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

static const httpd_uri_t index_page = {
    .uri          = "/",
    .method       = HTTP_GET,
    .handler      = index_page_get_handler,
    .user_ctx     = NULL};

static const httpd_uri_t post = {
    .uri          = "/",
    .method       = HTTP_POST,
    .handler      = post_handler,
    .user_ctx     = NULL,
};

static const httpd_uri_t style = {
    .uri          = "/style.css",
    .method       = HTTP_GET,
    .handler      = style_get_handler,
    .user_ctx     = NULL};

static const httpd_uri_t favicon = {
    .uri          = "/favicon.svg",
    .method       = HTTP_GET,
    .handler      = favicon_get_handler,
    .user_ctx     = NULL};

static const httpd_uri_t ws = {
    .uri          = "/ws",
    .method       = HTTP_GET,
    .handler      = ws_handler,
    .user_ctx     = NULL,
    .is_websocket = true
};

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
    config.max_open_sockets = MAX_WS_CONNECTIONS;
    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &index_page);
        httpd_register_uri_handler(server, &post);
        httpd_register_uri_handler(server, &style);
        httpd_register_uri_handler(server, &favicon);
        httpd_register_uri_handler(server, &ws);
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
