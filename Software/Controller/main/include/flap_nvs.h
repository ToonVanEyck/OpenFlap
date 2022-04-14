#ifndef FLAP_NVS_H
#define FLAP_NVS_H

#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"

#include "nvs_flash.h"
#include "nvs.h"

esp_err_t nvs_init();
esp_err_t flap_nvs_erase_key(char *field_key);
esp_err_t flap_nvs_get_string(char *field_key, char **field_value);
esp_err_t flap_nvs_set_string(char *field_key, char *field_value);

#endif