#ifndef UART_API_H
#define UART_API_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "flap_uart.h"
#include "chain_comm_abi.h"

void uart_api_init();

#endif