#include "flap_socket_server.h"

static const char *TAG = "[SOCK]";

static QueueHandle_t flap_socket_queue;
static TaskHandle_t flap_socket_taskHandle;
static vprintf_like_t original_log_func; 

int socket_log_func(const char *fmt, va_list args) {   
    char *log_msg;
    int ret = vasprintf(&log_msg,fmt,args);
    if(ret<0){
        printf("couldn't allocate memory for message...\n");
    }else{
	    if(xQueueSend(flap_socket_queue,&log_msg,50/portTICK_PERIOD_MS) != pdTRUE){
            free(log_msg);
        }
    }
    return ret;
}

static void flap_log_loop(const int sock)
{
    int len;
    char rx_buffer[128];
    char *tx_buffer;
    do {
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, MSG_DONTWAIT);
        if (len < 0) {
            if(errno != EAGAIN) len = 0;
        } else if (len == 0) {
            printf("Connection closed\n");
        } 
        if (xQueueReceive(flap_socket_queue, &tx_buffer,  50/portTICK_PERIOD_MS)){
            len = strlen(tx_buffer);
            int to_write = len;
            printf(tx_buffer);
            while (to_write > 0) {
                int written = send(sock, tx_buffer + (len - to_write), to_write, 0);
                if (written < 0) {
                    printf("Error occurred during sending: errno %d\n", errno);
                    to_write = 0;
                }
                to_write -= written;
            }
            free(tx_buffer);
        }

    } while (len != 0);
}

static void tcp_server_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;
    int keepAlive = 1;
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;
    struct sockaddr_storage dest_addr;

    if (addr_family == AF_INET) {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(PORT);
        ip_protocol = IPPROTO_IP;
    }

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1) {
        ESP_LOGI(TAG, "Socket listening");

        struct sockaddr_storage source_addr;
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }
        fcntl(sock,F_SETFL,O_NONBLOCK);

        // Set tcp keepalive option
        setsockopt(sock, SOL_SOCKET,  SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
        // Convert ip address to string
        if (source_addr.ss_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);


        original_log_func = esp_log_set_vprintf(socket_log_func);
        flap_log_loop(sock);
        esp_log_set_vprintf(original_log_func);

        shutdown(sock, 0);
        close(sock);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}

void flap_init_socket_server(void)
{
    flap_socket_queue = xQueueCreate(10, sizeof(char *));
    configASSERT(flap_socket_queue);
    xTaskCreate(tcp_server_task, "socket_server", 4096, (void*)AF_INET, 5, &flap_socket_taskHandle);
    configASSERT(flap_socket_taskHandle);
}

