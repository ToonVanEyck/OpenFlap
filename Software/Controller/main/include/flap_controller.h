#ifndef FLAP_H
#define FLAP_H

#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include "cJSON.h"

#include "flap_firmware.h"
#include "flap_nvs.h"
#include "flap_uart.h"
#include "flap_command.h"
#include "flap_http_server.h"

typedef struct{
    int rows;
    int cols;
}flap_ctx_t;

void controller_command_enqueue(controller_queue_data_t* cmd_comm);
void controller_respons_enqueue(controller_queue_data_t* cmd_comm);
void flap_controller_init();

void flap_module_get_dimensions(flap_ctx_t *flap_ctx);
void flap_module_get_message(flap_ctx_t *flap_ctx);
void flap_module_set_message(flap_ctx_t *flap_ctx, char *message);
void flap_module_goto_app(flap_ctx_t *flap_ctx);
void flap_module_get_charset(flap_ctx_t *flap_ctx);
void flap_module_set_charset(flap_ctx_t *flap_ctx, char *charset);
void flap_module_set_offset(flap_ctx_t *flap_ctx, char *offset);
void flap_controller_set_AP(flap_ctx_t *flap_ctx, char *data);
void flap_controller_set_STA(flap_ctx_t *flap_ctx, char *data);

#endif