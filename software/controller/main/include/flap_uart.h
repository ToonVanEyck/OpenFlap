#ifndef FLAP_UART_H
#define FLAP_UART_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chain_comm_abi.h"
#include "UartApi.h"
#include "Model.h"

#define UART_BUF_SIZE (1024)
#define UART_NUM UART_NUM_1
#define TX_PIN  (10)
#define RX_PIN  (9)
#define MAX_UART_API_ENDPOINTS 32
#define CMD_BUFF_SIZE 255
#define CMD_COMM_BUF_LEN 2048
#define EXTEND (0x80)

typedef union{
    struct{
        chainCommHeader_t commHeader;
        uint16_t size;
    }field;
    uint32_t raw;
}uartNotification_t;

typedef void (*uart_modulePropertyCallback_t)(char* data, module_t *module);

typedef struct{
    moduleProperty_t property;
    uart_modulePropertyCallback_t deserialize;
    uart_modulePropertyCallback_t serialize;
}uart_modulePropertyHandler_t;

void msg_init();
void msg_newReadAll(moduleProperty_t property);
void msg_newWriteAll(moduleProperty_t property);
void msg_newWriteSequential(moduleProperty_t property);
void msg_addHeader(moduleAction_t action, moduleProperty_t property);
void msg_addData(uint8_t byte);
void msg_send(const unsigned commandPeriod);
void msg_sendDoNothing(const unsigned commandPeriod);
inline void msg_sendAcknowledge(){msg_sendDoNothing(MAX_COMMAND_PERIOD_MS);}
   
bool uart_propertyReadAll(moduleProperty_t property);
bool uart_propertyWriteAll(moduleProperty_t property);
bool uart_propertyWriteSequential(moduleProperty_t property);
bool uart_moduleSerializedPropertiesAreEqual(moduleProperty_t property);

void uart_addModulePropertyHandler(moduleProperty_t property, uart_modulePropertyCallback_t deserialize, uart_modulePropertyCallback_t serialize);
void flap_uart_init();

TaskHandle_t uartTask();
#endif