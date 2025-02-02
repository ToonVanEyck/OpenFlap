#include "utils.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>

#define TAG "UTILS"

void util_extract_version(const char *version_str, uint8_t *major, uint8_t *minor, uint8_t *patch)
{
    char version_copy[32];
    strncpy(version_copy, version_str, sizeof(version_copy) - 1);
    version_copy[sizeof(version_copy) - 1] = '\0';

    char *token = strtok(version_copy, "v.-");
    if (token != NULL) {
        *major = atoi(token);
        token  = strtok(NULL, "v.-");
        if (token != NULL) {
            *minor = atoi(token);
            token  = strtok(NULL, "v.-");
            if (token != NULL) {
                *patch = atoi(token);
            } else {
                ESP_LOGE(TAG, "Failed to extract patch version");
            }
        } else {
            ESP_LOGE(TAG, "Failed to extract minor version");
        }
    } else {
        ESP_LOGE(TAG, "Failed to extract major version");
    }
}