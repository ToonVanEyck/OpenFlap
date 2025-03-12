#include "esp_log.h"
#include "memory_checks.h"
#include "networking.h"
#include "unity.h"
#include "webserver.h"

#define TAG "WEBSERVER_TEST"

esp_err_t api_post_handler(httpd_req_t *req);

TEST_CASE("Test webserver", "[webserver][qemu]")
{
    /* Configure Ethernet for testing on qemu. */
    networking_config_t config = NETWORKING_DEFAULT_CONFIG;
    networking_setup(&config);
    TEST_ASSERT_EQUAL(networking_wait_for_connection(10000), ESP_OK);

    webserver_ctx_t webserver_ctx;
    TEST_ASSERT_EQUAL(webserver_init(&webserver_ctx), ESP_OK);

    ESP_LOGI(TAG, "Webserver started");

    char dummy[16] = {0};
    unity_gets(dummy, sizeof(dummy));

    ESP_LOGI(TAG, "Webserver stopped");

    test_utils_record_free_mem();
}

//---------------------------------------------------------------------------------------------------------------------

TEST_CASE("Test web api", "[webserver][qemu]")
{
    /* Configure Ethernet for testing on qemu. */
    networking_config_t config = NETWORKING_DEFAULT_CONFIG;
    networking_setup(&config);
    TEST_ASSERT_EQUAL(networking_wait_for_connection(10000), ESP_OK);

    webserver_ctx_t webserver_ctx;
    TEST_ASSERT_EQUAL(webserver_init(&webserver_ctx), ESP_OK);

    webserver_api_method_handlers_t web_api_handlers = {
        .put_handler = api_post_handler,
    };

    ESP_LOGI(TAG, "Webserver started");

    TEST_ASSERT_EQUAL(
        ESP_OK, webserver_api_endpoint_add(&webserver_ctx, "/controller/firmware", &web_api_handlers, true, NULL));

    char dummy[16] = {0};
    unity_gets(dummy, sizeof(dummy));

    ESP_LOGI(TAG, "Webserver stopped");

    test_utils_record_free_mem();
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t api_post_handler(httpd_req_t *req)
{
    /* Attempt to allocate memory for posted data. */
    size_t read_cnt            = 0;
    const size_t max_read_size = 128;
    char buf[max_read_size];
    while (read_cnt < req->content_len) {
        size_t read_size = max_read_size;
        if (read_cnt + read_size > req->content_len) {
            read_size = req->content_len - read_cnt;
        }
        int ret = httpd_req_recv(req, buf, read_size);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                ESP_LOGE(TAG, "Socket timeout, retrying...");
                continue;
            }
            ESP_LOGE(TAG, "Failed to receive POST data");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, NULL);
            return ESP_FAIL;
        }
        read_cnt += ret;
        printf("%s\n", buf);
    }

    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}