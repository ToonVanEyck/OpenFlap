#ifndef FLAP_UART_H
#define FLAP_UART_H

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Model.h"
#include "UartApi.h"
#include "chain_comm_abi.h"

#define UART_BUF_SIZE          (1024)
#define UART_NUM               UART_NUM_1
#define TX_PIN                 (10)
#define RX_PIN                 (9)
#define MAX_UART_API_ENDPOINTS 32
#define CMD_BUFF_SIZE          255
#define CMD_COMM_BUF_LEN       2048
#define EXTEND                 (0x80)

typedef union {
    struct {
        chainCommHeader_t commHeader;
        uint16_t size;
    } field;
    uint32_t raw;
} uartNotification_t;

typedef void (*uart_modulePropertyCallback_t)(char *data, module_t *module);

typedef struct {
    property_id_t property;
    uart_modulePropertyCallback_t deserialize;
    uart_modulePropertyCallback_t serialize;
} uart_modulePropertyHandler_t;

void msg_init();
void msg_newReadAll(property_id_t property);
void msg_newWriteAll(property_id_t property);
void msg_newWriteSequential(property_id_t property);
void msg_addHeader(moduleAction_t action, property_id_t property);
void msg_addData(uint8_t byte);
void msg_send(const unsigned commandPeriod);
void msg_sendDoNothing(const unsigned commandPeriod);
inline void msg_sendAcknowledge()
{
    msg_sendDoNothing(MAX_COMMAND_PERIOD_MS);
}

bool uart_propertyReadAll(property_id_t property);
bool uart_propertyWriteAll(property_id_t property);
bool uart_propertyWriteSequential(property_id_t property);
bool uart_moduleSerializedPropertiesAreEqual(property_id_t property);

void uart_addModulePropertyHandler(property_id_t property, uart_modulePropertyCallback_t deserialize,
                                   uart_modulePropertyCallback_t serialize);
void flap_uart_init();

TaskHandle_t uartTask();
#endif