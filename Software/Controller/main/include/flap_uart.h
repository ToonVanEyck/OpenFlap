#ifndef FLAP_UART_H
#define FLAP_UART_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "flap_command.h"
#include "flap_api_v1.h"

#define UART_BUF_SIZE (1024)
#define UART_NUM UART_NUM_1
#define TX_PIN  (10)
#define RX_PIN  (9)

void flap_uart_init();
void flap_uart_send_data(char* data, int data_len);

#endif