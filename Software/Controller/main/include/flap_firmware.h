#ifndef FIRMWARE_H
#define FIRMWARE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "esp_wifi.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_log.h"
#include "flap_uart.h"

void flap_verify_controller_firmware();
void flap_controller_firmware_update(char *data,size_t data_len,size_t data_offset,size_t total_data_len);
void flap_module_firmware_update(char *data,size_t data_len,size_t data_offset,size_t total_data_len);
#endif