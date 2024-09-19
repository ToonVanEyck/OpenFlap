#include "flap_nvs.h"

static const char *TAG = "[NVS]";
static nvs_handle_t nvs_ctx;

esp_err_t nvs_init()
{
    esp_err_t err;
    err = nvs_flash_init();
    return err;
}

esp_err_t flap_nvs_erase_key(char *field_key)
{
    if (nvs_open("nvs", NVS_READWRITE, &nvs_ctx) == ESP_OK){
        if (nvs_erase_key(nvs_ctx, field_key) != ESP_OK){
            ESP_LOGE(TAG, "Failed to store %s",field_key);
            nvs_close(nvs_ctx);
            return ESP_FAIL;
        }
        if (nvs_commit(nvs_ctx) != ESP_OK){
            ESP_LOGI(TAG, "Failed to commit data to NVS");
            nvs_close(nvs_ctx);
            return ESP_FAIL;
        }
        nvs_close(nvs_ctx);
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t flap_nvs_get_string(char *field_key, char **field_value)
{
    if (nvs_open("nvs", NVS_READWRITE, &nvs_ctx) == ESP_OK){
        size_t required_size;
        nvs_get_str(nvs_ctx, field_key, NULL, &required_size);
        *field_value = malloc(required_size);
        if (nvs_get_str(nvs_ctx, field_key, *field_value, &required_size) != ESP_OK){
            ESP_LOGE(TAG,"Key %s not found",field_key);
            nvs_close(nvs_ctx);
            return ESP_FAIL;
        }
        nvs_close(nvs_ctx);
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t flap_nvs_set_string(char *field_key, char *field_value)
{
    if(nvs_open("nvs", NVS_READWRITE, &nvs_ctx) == ESP_OK){
        if (nvs_set_str(nvs_ctx, field_key, field_value) != ESP_OK){
            ESP_LOGE(TAG, "Failed to store %s",field_key);
            nvs_close(nvs_ctx);
            return ESP_FAIL;
        }
        if (nvs_commit(nvs_ctx) != ESP_OK){
            ESP_LOGI(TAG, "Failed to commit data to NVS");
            nvs_close(nvs_ctx);
            return ESP_FAIL;
        }
        nvs_close(nvs_ctx);
        return ESP_OK;
    }
    return ESP_FAIL;
}