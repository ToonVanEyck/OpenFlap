#include "flap_firmware.h"

static const char *TAG = "[FW]";

void flap_verify_controller_firmware()
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            // run diagnostic function ...
            uint8_t sha_256[32] = {0};
            if (esp_partition_get_sha256(running, sha_256) == ESP_OK) {
                ESP_LOGI(TAG, "Diagnostics for [patition:%s] completed successfully! Continuing execution ...",
                         running->label);
                esp_ota_mark_app_valid_cancel_rollback();
            } else {
                ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
    }
}

void flap_controller_firmware_update(char *data, size_t data_len, size_t data_offset, size_t total_data_len)
{
    static const esp_partition_t *update_partition;
    static esp_ota_handle_t update_handle;
    esp_err_t err;
    if (data_offset == 0) {
        ESP_LOGI(TAG, "STARTING OTA UPDATE!");
        update_handle    = 0;
        update_partition = esp_ota_get_next_update_partition(NULL);
        assert(update_partition != NULL);
        err = esp_ota_begin(update_partition, total_data_len, &update_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        }
    }
    ESP_LOGI(TAG, "OTA: writing %d %d/%d bytes", data_len, data_offset, total_data_len);
    err = esp_ota_write(update_handle, data, data_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error: esp_ota_write failed (%s)!", esp_err_to_name(err));
    }
    if (data_offset + data_len == total_data_len) { // last data
        err = esp_ota_end(update_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        }
        err = esp_ota_set_boot_partition(update_partition);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "firmware update failed");
        } else {
            ESP_LOGI(TAG, "firmware update successful");
            // esp_restart();
        }
    }
}

static uint8_t strtou8(char *str)
{
    char buf[3] = {0};
    strncpy(buf, str, 2);
    return strtol(buf, NULL, 16);
}

static uint16_t strtou16(char *str)
{
    char buf[5] = {0};
    strncpy(buf, str, 4);
    return strtol(buf, NULL, 16);
}

#define MODULE_FLASH_APP_SIZE   (28672)
#define MODULE_FLASH_PAGE_SIZE  (128)
#define MODULE_FLASH_PAGE_COUNT (MODULE_FLASH_APP_SIZE / MODULE_FLASH_PAGE_SIZE)

void flap_module_firmware_update(char *data, size_t data_len, size_t data_offset, size_t total_data_len)
{
    static char flash_page[MODULE_FLASH_PAGE_SIZE] = {0xFF};
    static uint8_t flash_page_offset               = 0;
    static uint8_t flash_page_index                = 0; /* The index of the page in the firmware. */
    static uint8_t flash_data_index                = 0; /* The index of the data in the page. */

    /* Clear offset if this is the start of the update. */
    if (data_offset == 0) {
        ESP_LOGI(TAG, "STARTING MODULE FIRMWARE UPDATE!");
        flash_page_index = 0;
    }

    for (size_t i = 0; i < data_len; i++) {
        flash_page[flash_data_index++] = data[i];
        if (flash_data_index == MODULE_FLASH_PAGE_SIZE) {
            flash_data_index = 0;
            // write flash_page to flash
            ESP_LOGI(TAG, "Writing firmware page %d / %d", flash_page_index, MODULE_FLASH_PAGE_COUNT);
            msg_newWriteAll(PROPERTY_FIRMWARE);
            msg_addData((flash_page_index >> 8) & 0xff);
            msg_addData((flash_page_index >> 0) & 0xff);
            for (int i = 0; i < MODULE_FLASH_PAGE_SIZE; i++) {
                msg_addData(flash_page[i]);
            }
            msg_addData(ACK);
            msg_send(200);
            flash_page_index++;
        }
    }

    if (data_offset + data_len == total_data_len) {
        ESP_LOGI(TAG, "Module firmware update complete!");
        ESP_LOGI(TAG, "Sending reboot command to modules ...");
        msg_newWriteAll(PROPERTY_COMMAND);
        msg_addData(CMD_REBOOT);
        msg_addData(ACK);
        msg_send(200);
    }
}

// static uint8_t hex_data_len = 0;
// static uint16_t addr = 0;
// static uint8_t cmd = 0;
// static char buf[45] = {0};
// static size_t buf_offset = 0;

// if (!data_offset) {
//     flash_data_cnt = 0;
//     hex_data_len = 0;
//     addr = 0;
//     cmd = 0;
//     // cs = 0;
//     bzero(buf, 45);
//     buf_offset = 0;

//     gpio_set_level(FLAP_ENABLE_PIN, 1);
//     vTaskDelay(50 / portTICK_PERIOD_MS);
//     // msg_newWriteAll(PROPERTY_COMMAND);
//     // msg_addData(CMD_REBOOT);
//     // msg_addData(ACK);
//     // msg_send(200);
//     // vTaskDelay(200 / portTICK_PERIOD_MS);
// }

// char *token = strtok(data, "\n");
// while (token != NULL) {
//     int data_remain = (data + data_len - token);
//     if (data_remain >= 43) {
//         data_remain = strlen(token);
//     }
//     strncat(buf + buf_offset, token, data_remain);
//     buf_offset = strlen(buf);
//     if (buf[0] == ':' && buf_offset >= 11) {
//         hex_data_len = strtou8(buf + 1);
//         cmd = strtou8(buf + 7);
//         if (cmd == 0) {
//             if (flash_data_cnt == 0) {
//                 addr = strtou16(buf + 3);
//             }
//             // cs = strtou8(buf+9+(2*hex_data_len));
//             if (11 + hex_data_len * 2 == buf_offset) {
//                 for (int i = 0; i < 32; i += 4) {
//                     if (i < hex_data_len * 2 && cmd == 0x00) {
//                         flash_data[flash_data_cnt++] = strtou8(buf + 9 + i);
//                         flash_data[flash_data_cnt++] = strtou8(buf + 11 + i);
//                     } else {
//                         flash_data[flash_data_cnt++] = 0x3F;
//                         flash_data[flash_data_cnt++] = 0xFF;
//                     }
//                 }
//                 if (flash_data_cnt == 64) {
//                     msg_newWriteAll(PROPERTY_FIRMWARE);
//                     msg_addData((addr >> 8) & 0xff);
//                     msg_addData((addr >> 0) & 0xff);
//                     for (int i = 0; i < 64; i++) {
//                         msg_addData(flash_data[i]);
//                     }
//                     msg_addData(ACK);
//                     msg_send(60);
//                     flash_data_cnt = 0;
//                     // cs = 0;
//                     cmd = 0;
//                 }
//                 buf_offset = 0;
//                 bzero(buf, sizeof(buf));
//             }
//         } else {
//             buf_offset = 0;
//             bzero(buf, sizeof(buf));
//             break;
//         }
//     }
//     token = strtok(NULL, "\n");
// }
// }