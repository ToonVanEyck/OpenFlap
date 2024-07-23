#define DO_GENERATE_PROPERTY_NAMES
#include "flap_http_server.h"

// #define MAX_WS_CONNECTIONS 3
static const char *TAG = "[HTTP]";
TaskHandle_t task;

static httpd_handle_t server = NULL;
http_modulePropertyHandler_t http_modulePropertyHandlers[end_of_properties] = {0};

typedef struct {
    size_t data_len;
    size_t data_offset;
    size_t total_data_len;
    char data[CMD_COMM_BUF_LEN];
} controller_queue_data_t;

static int httpd_req_recv_blocking(httpd_req_t *req, char *data, int len)
{
    int total_recv = 0, curr_recv = 0;
    do {
        curr_recv = httpd_req_recv(req, data + total_recv, len - total_recv);
        if (curr_recv < 0) {
            ESP_LOGE(TAG, "An error occured while reading data");
            return -curr_recv;
        } else if (curr_recv == 0) {
            ESP_LOGI(TAG, "There is tempoarily no more data.");
            taskYIELD();
        }
        total_recv += curr_recv;
    } while (total_recv < len);
    return total_recv;
}

static esp_err_t index_page_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_start, index_end - index_start);
    return ESP_OK;
}
static const httpd_uri_t index_page = {
    .uri = "/", .method = HTTP_GET, .handler = index_page_get_handler, .user_ctx = NULL};

typedef enum { update_none, update_module, update_controller } update_firmware_t;

static esp_err_t firmware_handler(httpd_req_t *req)
{
    static update_firmware_t update = update_none;
    controller_queue_data_t *controller_comm = calloc(1, sizeof(controller_queue_data_t));
    int read_cnt = 0;
    int boundry_length = 0;
    while (read_cnt <= 4 || controller_comm->data[read_cnt - 4] != '\r' ||
           controller_comm->data[read_cnt - 3] != '\n' || controller_comm->data[read_cnt - 2] != '\r' ||
           controller_comm->data[read_cnt - 1] != '\n') {
        if (httpd_req_recv_blocking(req, controller_comm->data + read_cnt, 1) <= 0) {
            return ESP_FAIL;
        }
        read_cnt++;
        if (!boundry_length && controller_comm->data[read_cnt - 2] == '\r' &&
            controller_comm->data[read_cnt - 1] == '\n')
            boundry_length = read_cnt + 4;
    }
    ESP_LOGI(TAG, "boundry len %d", boundry_length);
    ESP_LOGI(TAG, "%s", controller_comm->data);
    regex_t b;
    regmatch_t groupArray[5];
    b.re_magic = 0;
    int reti;
    reti = regcomp(&b, "name=\"([a-z_]*)\"", REG_EXTENDED);
    if (reti) {
        printf("Can't compile re\r\n");
    } else {
        reti = regexec(&b, controller_comm->data, 5, groupArray, 0);
        if (!reti) {
            for (int g = 0; g < 5; g++) {
                if (groupArray[g].rm_so == (size_t)-1)
                    break; // No more groups
                controller_comm->data[groupArray[g].rm_eo] = 0;
                if (g == 1) {
                    if (!strcmp(controller_comm->data + groupArray[g].rm_so, "controller_firmware")) {
                        update = update_controller;
                        ESP_LOGI(TAG, "command: update_controller");
                    } else if (!strcmp(controller_comm->data + groupArray[g].rm_so, "module_firmware")) {
                        update = update_module;
                        ESP_LOGI(TAG, "command: update_module");
                    } else {
                        ESP_LOGI(TAG, "command: undefined");
                    }
                }
            }
        }
    }
    regfree(&b);
    controller_comm->total_data_len = req->content_len - read_cnt - boundry_length;
    controller_comm->data_len = CMD_COMM_BUF_LEN;
    controller_comm->data_offset = 0;

    ESP_LOGI(TAG, "total useful data len %d, + boundry %d", controller_comm->total_data_len, req->content_len);
    ESP_LOGI(TAG, "already read %d, after data read %d", read_cnt, boundry_length);

    bzero(controller_comm->data, CMD_COMM_BUF_LEN);
    while (controller_comm->data_offset + controller_comm->data_len < controller_comm->total_data_len) {
        if (httpd_req_recv_blocking(req, controller_comm->data, controller_comm->data_len) <= 0) {
            return ESP_FAIL;
        }
        if (update == update_controller) {
            flap_controller_firmware_update(controller_comm->data, controller_comm->data_len,
                                            controller_comm->data_offset, controller_comm->total_data_len);
        } else if (update == update_module) {
            flap_module_firmware_update(controller_comm->data, controller_comm->data_len, controller_comm->data_offset,
                                        controller_comm->total_data_len);
        }
        controller_comm->data_offset += controller_comm->data_len;
    }

    controller_comm->data_len = controller_comm->total_data_len - controller_comm->data_offset;
    if (controller_comm->data_len) {
        if (httpd_req_recv_blocking(req, controller_comm->data, controller_comm->data_len) <= 0) {
            return ESP_FAIL;
        }
        if (update == update_controller) {
            flap_controller_firmware_update(controller_comm->data, controller_comm->data_len,
                                            controller_comm->data_offset, controller_comm->total_data_len);
        } else if (update == update_module) {
            flap_module_firmware_update(controller_comm->data, controller_comm->data_len, controller_comm->data_offset,
                                        controller_comm->total_data_len);
        }
        controller_comm->data_offset += controller_comm->data_len;
    }
    if (httpd_req_recv_blocking(req, controller_comm->data, boundry_length) <= 0) {
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "%.*s", boundry_length, controller_comm->data);

    free(controller_comm);

    httpd_resp_set_status(req, "204 No Content");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, NULL, 0);

    if (update == update_controller) {
        vTaskDelay(50 / portTICK_PERIOD_MS);
        esp_restart();
    }

    return ESP_OK;
}
static const httpd_uri_t firmware_uri = {
    .uri = "/firmware",
    .method = HTTP_POST,
    .handler = firmware_handler,
    .user_ctx = NULL,
};

static esp_err_t style_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)style_start, style_end - style_start);
    return ESP_OK;
}
static const httpd_uri_t style_uri = {
    .uri = "/style.css", .method = HTTP_GET, .handler = style_get_handler, .user_ctx = NULL};

static esp_err_t favicon_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "image/svg+xml");
    httpd_resp_send(req, (const char *)favicon_start, favicon_end - favicon_start);
    return ESP_OK;
}

static const httpd_uri_t favicon_uri = {
    .uri = "/favicon.svg", .method = HTTP_GET, .handler = favicon_get_handler, .user_ctx = NULL};

static esp_err_t script_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/javascript");
    httpd_resp_send(req, (const char *)script_start, script_end - script_start);
    return ESP_OK;
}

static const httpd_uri_t script_uri = {
    .uri = "/script.js", .method = HTTP_GET, .handler = script_get_handler, .user_ctx = NULL};

void http_addModulePropertyHandler(moduleProperty_t property, http_modulePropertyCallback_t toJson,
                                   http_modulePropertyCallback_t fromJson)
{
    if (property <= no_property && property >= end_of_properties) {
        ESP_LOGE(TAG, "Cannot add invalid property %d", property);
        return;
    }
    http_modulePropertyHandlers[property].toJson = toJson;
    http_modulePropertyHandlers[property].fromJson = fromJson;
}

esp_err_t api_get_http_modulePropertyHandlers(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET request on %s", req->uri);
    ulTaskNotifyTake(true, 0);
    char buf[MAX_HTTP_BODY_SIZE] = {0};
    strcpy(buf, "[");

    for (moduleProperty_t p = no_property + 1; p < end_of_properties; p++) {
        if (http_modulePropertyHandlers[p].toJson) {
            display_requestModuleProperty(p);
        }
    }
    uint64_t requestedProperties = display_getRequestModuleProperties();

    // process
    model_preformUart();

    // populate json
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    httpd_resp_set_type(req, "application/json");
    if (display_getSize()) {
        for (size_t i = 0; i < display_getSize();) {
            ESP_LOGI(TAG, "getting info of module %d", i);
            cJSON *json = cJSON_CreateObject();
            module_t *module = display_getModule(i);
            if (!module) {
                ESP_LOGE(TAG, "display object is missing module %d", i);
                continue;
            }
            cJSON_AddNumberToObject(json, "module", i);
            for (moduleProperty_t p = no_property + 1; p < end_of_properties; p++) {
                cJSON *property = NULL;
                if (requestedProperties & (1 << p) && http_modulePropertyHandlers[p].toJson) {
                    http_modulePropertyHandlers[p].toJson(&property, module);
                    cJSON_AddItemToObject(json, get_property_name(p), property);
                }
            }

            size_t buf_offset = strlen(buf);
            if (!cJSON_PrintPreallocated(json, buf + buf_offset, MAX_HTTP_BODY_SIZE - buf_offset - 10, true)) {
                memset(buf + buf_offset, 0, MAX_HTTP_BODY_SIZE - buf_offset);
                httpd_resp_send_chunk(req, (const char *)buf, strlen(buf));
                memset(buf, 0, MAX_HTTP_BODY_SIZE);
            } else if (i == display_getSize() - 1) {
                strcat(buf, "]");
                httpd_resp_send_chunk(req, (const char *)buf, strlen(buf));
                memset(buf, 0, MAX_HTTP_BODY_SIZE);
                i++;
            } else {
                strcat(buf, ",");
                i++;
            }
            cJSON_Delete(json);
        }
        httpd_resp_send_chunk(req, NULL, 0);
    } else {
        httpd_resp_send(req, "[]", 2);
    }
    return ESP_OK;
}
esp_err_t api_set_http_modulePropertyHandlers(httpd_req_t *req)
{
    ESP_LOGI(TAG, "POST request on %s", req->uri);
    ulTaskNotifyTake(true, 0);
    char buf[MAX_HTTP_BODY_SIZE] = {0};
    size_t to_parse_size = 0;
    int recv_cnt = 0;
    // Receive data. Only a json array is starting with '[' at index 0 is accepted. A large json can be read an parsed
    // in multiple sections.
    while ((recv_cnt = httpd_req_recv(req, buf + to_parse_size, MAX_HTTP_BODY_SIZE - 1 - to_parse_size))) {
        size_t json_last_object_end = 0;
        size_t json_first_object_start = 0;
        size_t json_object_token_cnt = 0;

        if (buf[0] != '[') {
            httpd_resp_set_status(req, "422 Unprocessable Entity");
            httpd_resp_send(req, NULL, 0);
            return ESP_OK;
        }

        for (int i = 0; i < MAX_HTTP_BODY_SIZE - 1; i++) {
            if (json_object_token_cnt == 0 && buf[i] == ']') {
                break;
            } else if (buf[i] == '{') {
                if (++json_object_token_cnt == 1 && json_first_object_start == 0) {
                    json_first_object_start = i;
                }
            } else if (buf[i] == '}') {
                if (--json_object_token_cnt == 0) {
                    json_last_object_end = i;
                }
            }
        }

        if (json_first_object_start == 0 || json_first_object_start > json_last_object_end ||
            json_last_object_end == MAX_HTTP_BODY_SIZE) {
            httpd_resp_set_status(req, "422 Unprocessable Entity");
            httpd_resp_send(req, NULL, 0);
            return ESP_OK;
        }
        buf[json_last_object_end + 1] = ']';
        buf[json_first_object_start - 1] = '[';

        // parse object
        to_parse_size = to_parse_size + recv_cnt - json_last_object_end - 1;

        cJSON *json = cJSON_ParseWithLength(buf, json_last_object_end + 2);
        if (!json || !cJSON_IsArray(json)) {
            ESP_LOGE(TAG, "Failed to parse json");
            httpd_resp_set_status(req, "500 Internal Server Error");
            httpd_resp_send(req, NULL, 0);
            return ESP_OK;
        }

        cJSON *json_module = NULL;
        cJSON_ArrayForEach(json_module, json)
        {
            cJSON *json_module_index = cJSON_GetObjectItemCaseSensitive(json_module, "module");
            if (!cJSON_IsNumber(json_module_index)) {
                ESP_LOGE(TAG, "no module specified, ignoring this object");
                cJSON_Delete(json);
                httpd_resp_set_status(req, "422 Unprocessable Entity");
                httpd_resp_send(req, NULL, 0);
                return ESP_OK;
            }
            module_t *module = display_getModule(json_module_index->valueint);
            if (!module) {
                ESP_LOGE(TAG, "module %d is invalid, ignoring this object", json_module_index->valueint);
                cJSON_Delete(json);
                httpd_resp_set_status(req, "422 Unprocessable Entity");
                httpd_resp_send(req, NULL, 0);
                return ESP_OK;
            }
            char *string = cJSON_Print(json_module);
            ESP_LOGI(TAG, "%s", string);
            free(string);
            for (moduleProperty_t p = no_property + 1; p < end_of_properties; p++) {
                cJSON *property = cJSON_GetObjectItemCaseSensitive(json_module, get_property_name(p));
                if (property && http_modulePropertyHandlers[p].fromJson) {
                    if (!http_modulePropertyHandlers[p].fromJson(&property, module)) {
                        cJSON_Delete(json);
                        httpd_resp_set_status(req, "422 Unprocessable Entity");
                        httpd_resp_send(req, NULL, 0);
                        return ESP_OK;
                    }
                }
            }
        }
        cJSON_Delete(json);
        memcpy(buf + 1, buf + json_last_object_end + 2, to_parse_size - 1);
        memset(buf + to_parse_size, 0, MAX_HTTP_BODY_SIZE - to_parse_size);
    }

    xTaskNotify(modelTask(), fromHttp, eSetValueWithoutOverwrite);
    uint32_t event = ulTaskNotifyTake(true, 10000 / portTICK_RATE_MS);
    if (!event) {
        ESP_LOGE(TAG, "Controller has not responded.");
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }

    httpd_resp_set_status(req, "200 OK");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t api_get_module_endpoint = {
    .uri = "/api/modules", .method = HTTP_GET, .handler = api_get_http_modulePropertyHandlers, .user_ctx = NULL};

static const httpd_uri_t api_set_module_endpoint = {
    .uri = "/api/modules", .method = HTTP_POST, .handler = api_set_http_modulePropertyHandlers, .user_ctx = NULL};

esp_err_t sendCrossOriginHeader(httpd_req_t *req)
{
    httpd_resp_set_status(req, "204 No Content");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t api_option_module_endpoint = {
    .uri = "/api/modules", .method = HTTP_OPTIONS, .handler = sendCrossOriginHeader, .user_ctx = NULL};

static esp_err_t wifiCredentials_post_handler(httpd_req_t *req)
{
    LARGE_REQUEST_GUARD(req);
    // Receive data.
    char *data = calloc(1, CMD_COMM_BUF_LEN);
    if (httpd_req_recv(req, data, req->content_len) <= 0) {
        free(data);
        return ESP_FAIL;
    }
    data[req->content_len] = 0;
    ESP_LOGI(TAG, "%s", data);
    bool joinAP;
    // Parse json.
    cJSON *json = cJSON_Parse(data);

    cJSON *type = cJSON_GetObjectItemCaseSensitive(json, "host");
    if (type) {
        joinAP = false;
    } else if ((type = cJSON_GetObjectItemCaseSensitive(json, "join"))) {
        joinAP = true;
    } else {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, NULL, 0);
        cJSON_Delete(json);
        free(data);
        return ESP_OK;
    }

    cJSON *ssid = cJSON_GetObjectItemCaseSensitive(type, "ssid");
    cJSON *pwd = cJSON_GetObjectItemCaseSensitive(type, "password");

    if (!(cJSON_IsString(ssid) && cJSON_IsString(pwd))) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, NULL, 0);
        cJSON_Delete(json);
        free(data);
        return ESP_OK;
    }
    // Store Wi-Fi credentials in NVM, They will be used when the device reboots.
    if (joinAP) {
        flap_nvs_set_string("STA_ssid", ssid->valuestring);
        flap_nvs_set_string("STA_pwd", pwd->valuestring);
    } else {
        flap_nvs_set_string("AP_ssid", ssid->valuestring);
        flap_nvs_set_string("AP_pwd", pwd->valuestring);
    }
    // Send response to client.
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_send(req, NULL, 0);
    cJSON_Delete(json);
    free(data);
    return ESP_OK;
}

static const httpd_uri_t wifi_ap_post = {
    .uri = "/api/wifi", .method = HTTP_POST, .handler = wifiCredentials_post_handler, .user_ctx = NULL};

void flap_init_webserver()
{
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &http_server_connect_handler, NULL));
    ESP_ERROR_CHECK(
        esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &http_server_disconnect_handler, NULL));
    ESP_ERROR_CHECK(
        esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &http_server_connect_handler, NULL));
}

httpd_handle_t flap_start_webserver(void)
{
    // httpd_handle_t server;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size += MAX_HTTP_BODY_SIZE;
    config.max_uri_handlers = 20;
    // config.stack_size = 8000;
    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &index_page);
        httpd_register_uri_handler(server, &firmware_uri);
        httpd_register_uri_handler(server, &style_uri);
        httpd_register_uri_handler(server, &favicon_uri);
        httpd_register_uri_handler(server, &script_uri);
        httpd_register_uri_handler(server, &wifi_ap_post);
        httpd_register_uri_handler(server, &api_get_module_endpoint);
        httpd_register_uri_handler(server, &api_set_module_endpoint);
        httpd_register_uri_handler(server, &api_option_module_endpoint);
        // httpd_register_uri_handler(server, &ws);
        http_moduleEndpointInit();
        return server;
    }
    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void stop_webserver()
{
    httpd_stop(server);
}

void http_server_disconnect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver();
        server = NULL;
    }
}

void http_server_connect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        server = flap_start_webserver();
        task = xTaskGetHandle("httpd");
    }
}

TaskHandle_t httpTask()
{
    return task;
}