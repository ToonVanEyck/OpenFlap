#pragma once

#include "chain_comm_abi.h"
#include "display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct {
    display_t *display; /**< Display context. */
    TaskHandle_t task;  /**< Task handle. */
} chain_comm_ctx_t;