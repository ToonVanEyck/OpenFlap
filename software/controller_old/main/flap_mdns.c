#include "flap_mdns.h"

static const char *TAG = "[MDNS]";

void flap_mdns_init(char* hostname)
{
    ESP_ERROR_CHECK(mdns_init());
    ESP_LOGI(TAG, "mdns hostname set to: [%s]", hostname);
    ESP_ERROR_CHECK(mdns_hostname_set(hostname));
    mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);

    char *delegated_hostname;
    if (-1 == asprintf(&delegated_hostname, "%s-delegated", hostname)) abort();

    mdns_ip_addr_t addr4, addr6;
    esp_netif_str_to_ip4("10.0.0.1", &addr4.addr.u_addr.ip4);
    addr4.addr.type = ESP_IPADDR_TYPE_V4;
    esp_netif_str_to_ip6("fd11:22::1", &addr6.addr.u_addr.ip6);
    addr6.addr.type = ESP_IPADDR_TYPE_V6;
    addr4.next = &addr6;
    addr6.next = NULL;
    ESP_ERROR_CHECK( mdns_delegate_hostname_add(delegated_hostname, &addr4) );
    ESP_ERROR_CHECK( mdns_service_add_for_host(NULL, "_http", "_tcp", delegated_hostname, 80, NULL, 0) );
    free(delegated_hostname);
}