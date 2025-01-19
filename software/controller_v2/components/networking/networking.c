#include "networking.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_eth.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <string.h>

#define TAG "NETWORKING"

#define WIFI_CONNECTED_BIT BIT0
#define ETH_CONNECTED_BIT  BIT1

static EventGroupHandle_t networking_events = NULL;

static void networking_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

static esp_err_t networking_wifi_setup(const networking_config_t *config);

static esp_err_t networking_ethernet_setup(const networking_config_t *config);

static void wifi_re_connect(uint8_t *retry_count);

//---------------------------------------------------------------------------------------------------------------------

esp_err_t networking_setup(const networking_config_t *config)
{
    /* Validate that networking has not been configured yet. */
    ESP_RETURN_ON_FALSE(networking_events == NULL, ESP_FAIL, TAG, "Networking already configured.");

    networking_events = xEventGroupCreate();
    ESP_RETURN_ON_FALSE(networking_events != NULL, ESP_FAIL, TAG, "Failed to create networking event group.");

    /* Initialize TCP/IP network interface (should be called only once in application). */
    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "Failed to initialize esp-netif.");
    /* Create default event loop. */
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "Failed to create default event loop.");

    /* Register event handlers. */
    ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &networking_event_handler, NULL), TAG,
                        "Failed to register IP event handler.");

    if (config->wifi.access_point.enable || config->wifi.station.enable) {
        ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &networking_event_handler, NULL),
                            TAG, "Failed to register wifi event handler.");
        ESP_RETURN_ON_ERROR(networking_wifi_setup(config), TAG, "Failed to setup wifi.");
    }

    if (config->ethernet.enable) {
        ESP_RETURN_ON_ERROR(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &networking_event_handler, NULL),
                            TAG, "Failed to register ethernet event handler.");
        ESP_RETURN_ON_ERROR(networking_ethernet_setup(config), TAG, "Failed to setup ethernet.");
    }
    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t networking_wait_for_connection(uint32_t timeout_ms)
{
    EventBits_t bits = xEventGroupWaitBits(networking_events, WIFI_CONNECTED_BIT | ETH_CONNECTED_BIT, pdFALSE, pdFALSE,
                                           pdMS_TO_TICKS(timeout_ms));
    if (bits & (WIFI_CONNECTED_BIT | ETH_CONNECTED_BIT)) {
        return ESP_OK;
    } else {
        return ESP_ERR_TIMEOUT;
    }
}

//---------------------------------------------------------------------------------------------------------------------

static void networking_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    static uint8_t wifi_connection_attempt_cnt = 0;
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi Started");
                wifi_re_connect(&wifi_connection_attempt_cnt);
                break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "ESP connected to AP");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "ESP disconnected from AP");
                wifi_re_connect(&wifi_connection_attempt_cnt);
                break;
            case WIFI_EVENT_AP_STACONNECTED:
                ESP_LOGI(TAG, "Client connected to ESP");
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:
                ESP_LOGI(TAG, "Client disconnected from ESP");
                break;
            default:
                break;
        }
    } else if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case ETHERNET_EVENT_START:
                ESP_LOGI(TAG, "Ethernet Started");
                break;
            case ETHERNET_EVENT_CONNECTED:
                ESP_LOGI(TAG, "Ethernet Connected");
                break;
            case ETHERNET_EVENT_DISCONNECTED:
                ESP_LOGI(TAG, "Ethernet Disconnected");
                break;
            default:
                break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP:
                ESP_LOGI(TAG, "Wifi interface got IP address");
                xEventGroupSetBits(networking_events, WIFI_CONNECTED_BIT);
                break;
            case IP_EVENT_STA_LOST_IP:
                ESP_LOGI(TAG, "Wifi interface lost IP address");
                xEventGroupClearBits(networking_events, WIFI_CONNECTED_BIT);
                break;
            case IP_EVENT_ETH_GOT_IP:
                ESP_LOGI(TAG, "Ethernet interface got IP address");
                xEventGroupSetBits(networking_events, ETH_CONNECTED_BIT);
                break;
            case IP_EVENT_ETH_LOST_IP:
                ESP_LOGI(TAG, "Ethernet interface lost IP address");
                xEventGroupClearBits(networking_events, ETH_CONNECTED_BIT);
                break;
            default:
                break;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

static esp_err_t networking_wifi_setup(const networking_config_t *config)
{
    /* Initialize NVS, required for using wifi. */
    ESP_RETURN_ON_ERROR(nvs_flash_init(), TAG, "Failed to initialize NVS.");

    /* Validate credentials. */
    if (config->wifi.access_point.enable) {
        ESP_RETURN_ON_FALSE(strlen(config->wifi.access_point.ssid) >= 8, ESP_ERR_INVALID_ARG, TAG,
                            "Access point ssid to short.");
        ESP_RETURN_ON_FALSE(strlen(config->wifi.access_point.password) >= 8, ESP_ERR_INVALID_ARG, TAG,
                            "Access point password to short.");
    }

    if (config->wifi.station.enable) {
        ESP_RETURN_ON_FALSE(strlen(config->wifi.station.ssid) >= 8, ESP_ERR_INVALID_ARG, TAG, "Station ssid to short.");
        ESP_RETURN_ON_FALSE(strlen(config->wifi.station.password) >= 8, ESP_ERR_INVALID_ARG, TAG,
                            "Station password to short.");
    }

    /* Init wifi. */
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&wifi_init_config), TAG, "Failed to initialize wifi.");

    /* Set wifi mode. */
    wifi_mode_t wifi_mode = WIFI_MODE_NULL;
    wifi_mode |= config->wifi.station.enable ? WIFI_MODE_STA : WIFI_MODE_NULL;
    wifi_mode |= config->wifi.access_point.enable ? WIFI_MODE_AP : WIFI_MODE_NULL;
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(wifi_mode), TAG, "Failed to set wifi mode.");

    /* Configure wifi station*/
    if (config->wifi.station.enable) {
        esp_netif_create_default_wifi_sta();
        wifi_config_t wifi_config = {
            .sta =
                {
                    .ssid               = {0},
                    .password           = {0},
                    .threshold.authmode = WIFI_AUTH_WPA2_PSK,
                    .pmf_cfg            = {.capable = true, .required = false},
                },
        };
        if (config->wifi.station.ssid) {
            strcpy((char *)wifi_config.sta.ssid, config->wifi.station.ssid);
        }
        if (config->wifi.station.password) {
            strcpy((char *)wifi_config.sta.password, config->wifi.station.password);
        }
        ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG, "Failed to set wifi station config.");
    }

    /* Configure wifi access point */
    if (config->wifi.access_point.enable) {
        esp_netif_create_default_wifi_ap();
        wifi_config_t wifi_config = {
            .ap =
                {
                    .ssid            = {0},
                    .password        = {0},
                    .channel         = 1,
                    .authmode        = WIFI_AUTH_WPA2_PSK,
                    .beacon_interval = 400,
                    .max_connection  = 5,
                },
        };
        if (config->wifi.access_point.ssid) {
            strcpy((char *)wifi_config.sta.ssid, config->wifi.access_point.ssid);
        }
        if (config->wifi.access_point.password) {
            strcpy((char *)wifi_config.sta.password, config->wifi.access_point.password);
        }
        ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &wifi_config), TAG,
                            "Failed to set wifi access point config.");
    }

    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Failed to start wifi.");

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

static esp_err_t networking_ethernet_setup(const networking_config_t *config)
{
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();

    esp_eth_mac_t *mac = NULL;
    esp_eth_phy_t *phy = NULL;

#if CONFIG_ETH_USE_OPENETH
    /* Used for qemu */
    mac = esp_eth_mac_new_openeth(&mac_config);
    phy = esp_eth_phy_new_dp83848(&phy_config);
#else
    /* No other ethernet configurations supported. */
    (void)mac_config;
    (void)phy_config;
    return ESP_ERR_NOT_SUPPORTED;
#endif

    /* Install ethernet driver. */
    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle;
    ESP_RETURN_ON_ERROR(esp_eth_driver_install(&eth_config, &eth_handle), TAG, "Failed to install ethernet driver.");

    /* Apply net interface glue logic. */
    esp_netif_config_t netif_config                   = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif                            = esp_netif_new(&netif_config);
    esp_eth_netif_glue_handle_t eth_netif_glue_handle = esp_eth_new_netif_glue(eth_handle);
    ESP_RETURN_ON_ERROR(esp_netif_attach(eth_netif, eth_netif_glue_handle), TAG, "Failed to attach ethernet netif.");

    /* Start ethernet. */
    ESP_RETURN_ON_ERROR(esp_eth_start(eth_handle), TAG, "Failed to start ethernet.");
    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

static void wifi_re_connect(uint8_t *retry_count)
{
    const uint8_t retry_max = 10;
    if (*retry_count < retry_max) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Connecting to wifi access point, attempt %d", *retry_count);
        (*retry_count)++;
    } else {
        ESP_LOGW(TAG, "No longer attempting to reconnect to wifi access point, tried %d times", *retry_count);
    }
}