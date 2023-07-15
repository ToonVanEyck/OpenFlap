#include "flap_uart.h"

static const char *TAG = "[UART]";

static TaskHandle_t task;

static chainCommMessage_t msg;

static uart_modulePropertyHandler_t uart_modulePropertyHandlers[MAX_PROPERTIES] = {0};

void msg_init(){
    memset(&msg,0,sizeof(chainCommMessage_t));
}

void msg_newReadAll(moduleProperty_t property){
    msg_init();
    msg_addHeader(property_readAll,property);
    msg_addData(0); // add module indexer
    msg_addData(0); // add module indexer
}

void msg_newWriteSequential(moduleProperty_t property){
    msg_init();
    msg_addHeader(property_writeSequential,property);
}

void msg_newWriteAll(moduleProperty_t property){
    msg_init();
    msg_addHeader(property_writeAll,property);
}

void msg_addHeader(moduleAction_t action, moduleProperty_t property){
    if(msg.size > 0){
        ESP_LOGE(TAG,"Message is not empty");
        return;
    }
    msg.structured.header.field.action = action;
    msg.structured.header.field.property = property;
    msg.size++;
}

void msg_addData(uint8_t byte){
    if(msg.size >= CHAIN_COM_MAX_LEN){
        ESP_LOGE(TAG,"Message buffer is full");
        return;
    }
    msg.raw[msg.size++] = byte;
}

void msg_sendDoNothing(const unsigned commandPeriod){
    msg_init();
    msg_addHeader(do_nothing,no_property);
    msg_send(commandPeriod);
}

void msg_send(const unsigned commandPeriod){
    static TickType_t xLastWakeTime = 0;
    if(!msg.size || msg.size >= CHAIN_COM_MAX_LEN){
        ESP_LOGE(TAG,"Message size (%d) is invalid",msg.size);
        return;
    }
    if(commandPeriod){
        xTaskDelayUntil( &xLastWakeTime, commandPeriod / portTICK_RATE_MS );
    }
    char *buf = calloc(1,msg.size*5*sizeof(char)+1);
    for(int i = 0;i<msg.size;i++){
        sprintf(buf+5*i,"0x%02x ",msg.raw[i]);
    }
    ESP_LOGI(TAG,"TX --> %s",buf);
    free(buf);

    xTaskNotify(uartTask(), msg.structured.header.raw, eSetValueWithoutOverwrite);
    for(int i=0; i<msg.size;){
        i+= uart_write_bytes(UART_NUM,msg.raw+i, 1);
        esp_rom_delay_us(200);
    }
    xLastWakeTime = xTaskGetTickCount();
}

void uart_addModulePropertyHandler(moduleProperty_t property, uart_modulePropertyCallback_t deserialize, uart_modulePropertyCallback_t serialize){
    if(property <= no_property && property >= end_of_properties){
        ESP_LOGE(TAG,"Cannot add invalid property %d",property);
        return;
    }
    uart_modulePropertyHandlers[property].deserialize = deserialize;
    uart_modulePropertyHandlers[property].serialize = serialize;
}

bool uart_propertyReadAll(moduleProperty_t property){
    if((property <= no_property && property >= end_of_properties) || 
        !uart_modulePropertyHandlers[property].deserialize){
        ESP_LOGE(TAG,"No deserialization defined for property %d",property);
        return false;
    }
    msg_newReadAll(property);
    msg_send(MAX_COMMAND_PERIOD_MS);
    ulTaskNotifyTake(true, 5000 / portTICK_RATE_MS); // wait for command to finish
    return true;
}

bool uart_propertyWriteAll(moduleProperty_t property){
    if((property <= no_property && property >= end_of_properties) || 
        !uart_modulePropertyHandlers[property].serialize || !display_getSize()){
        ESP_LOGE(TAG,"No serialization defined for property %d",property);
        return false;
    }
    for(int i = 0;i<display_getSize();i++){
        module_t *module = display_getModule(i);
        module->updatableProperties &= ~(1<<property);
    }
    msg_newWriteAll(property);
    uart_modulePropertyHandlers[property].serialize(&msg.raw[msg.size],display_getModule(0));
    msg.size += propertySizes[property];
    msg_addData(ACK);
    msg_send(MAX_COMMAND_PERIOD_MS);
    ulTaskNotifyTake(true, 5000 / portTICK_RATE_MS); // wait for command to finish
    return true;
}

bool uart_propertyWriteSequential(moduleProperty_t property){
    if((property <= no_property && property >= end_of_properties) || 
        !uart_modulePropertyHandlers[property].serialize){
        ESP_LOGE(TAG,"No serialization defined for property %d",property);
        return false;
    }
    for(int i = 0;i<display_getSize();i++){
        module_t *module = display_getModule(i);
        if(module->updatableProperties & (1<<property)){
            module->updatableProperties &= ~(1<<property);
            msg_newWriteSequential(property);
            uart_modulePropertyHandlers[property].serialize(&msg.raw[msg.size],module);
            msg.size += propertySizes[property];
            msg_send(0);
        }else{
            msg_newWriteSequential(no_property);
            msg_send(0);
        }
    }
    msg_sendAcknowledge();
    ulTaskNotifyTake(true, 5000 / portTICK_RATE_MS); // wait for command to finish
    return true;
}

bool uart_moduleSerializedPropertiesAreEqual(moduleProperty_t property)
{
    if(display_getSize() < 2 && uart_modulePropertyHandlers[property].serialize){
        return true;
    }
    char bufA[CHAIN_COM_MAX_LEN] = {0};
    uart_modulePropertyHandlers[property].serialize(bufA,display_getModule(0));
    char bufB[CHAIN_COM_MAX_LEN] = {0};
    for(int i = 1;i<display_getSize();i++){
        module_t *module = display_getModule(i);
        uart_modulePropertyHandlers[property].serialize(bufB,display_getModule(i));
        for(int j = 0; j<propertySizes[property]; j++){
            if(bufA[j] != bufB[j]){
                return false;
            }
        }
    }
    return true;
}

uint32_t uart_receive(char *buf, uint32_t length, TickType_t ticks_to_wait){
    uint32_t len = uart_read_bytes(UART_NUM, buf, length, ticks_to_wait);
    if(len > 0 && len <= length){
        char *print_buf = calloc(len*5+1,1);
        for(int i = 0;i<len;i++){
            sprintf(print_buf+5*i,"0x%02x ",buf[i]);
        }
        ESP_LOGI(TAG,"RX <-- %s",print_buf);
        free(print_buf);
    }
    return len;
}

static void flap_uart_task(void *arg)
{
    uint32_t len = 0;
    uint16_t module_total = 0, module_index = 0;
    bool waitingForWriteSequentialAck = false;
    char buf[CHAIN_COM_MAX_LEN] = {0};

    chainCommHeader_t header;
    while(1){
        header.raw = (uint8_t)ulTaskNotifyTake(true, 250 / portTICK_RATE_MS);
        switch(header.field.action){
            case property_writeAll:
                len = uart_receive(buf, propertySizes[header.field.property] + WRITE_HEADER_LEN + 1, 250 / portTICK_RATE_MS);
                if(len != propertySizes[header.field.property] + WRITE_HEADER_LEN + 1){
                    ESP_LOGE(TAG,"Received %ld bytes but expected %d bytes for this \"writeAll\" command.",len,propertySizes[header.field.property] + WRITE_HEADER_LEN + 1);
                    break;
                }
                xTaskAbortDelay(modelTask()); // allow the uart port to be used again without delay.
                xTaskNotify(modelTask(), fromUart, eSetValueWithoutOverwrite);
                break;
            case property_readAll:
                len = uart_receive(buf, READ_HEADER_LEN, 250 / portTICK_RATE_MS);

                if(len != READ_HEADER_LEN){
                    ESP_LOGE(TAG,"Received %ld bytes but expected a %d byte \"readAll\" header.",len,READ_HEADER_LEN);
                    break;
                }
                module_total = buf[1] + buf[2] * 0xff;
                display_setSize(module_total);
                
                ESP_LOGI(TAG,"Expecting %d bytes from %d modules",propertySizes[header.field.property], module_total);         
                for(module_index = 0; module_index < module_total; module_index++){
                    len = uart_receive(buf, propertySizes[header.field.property], 250 / portTICK_RATE_MS);
                    if(len != propertySizes[header.field.property]){
                        ESP_LOGE(TAG,"Received %ld bytes but expected %d bytes from this property",len,propertySizes[header.field.property]);
                        break;
                    }
                    module_t* module = display_getModule(module_index);
                    if(uart_modulePropertyHandlers[header.field.property].deserialize && module){
                        uart_modulePropertyHandlers[header.field.property].deserialize(buf, module);
                    }
                    module->updatableProperties = 0; // don't update the modules again.
                }
                xTaskAbortDelay(modelTask()); // allow the uart port to be used again without delay.
                xTaskNotify(modelTask(), fromUart, eSetValueWithoutOverwrite);
                break;
            case property_writeSequential:
                waitingForWriteSequentialAck = true;
                break;
            default:
                if(waitingForWriteSequentialAck){
                    waitingForWriteSequentialAck = 0;
                    len = uart_receive(buf, 1, 250 / portTICK_RATE_MS);
                    if(len != 1){
                        ESP_LOGE(TAG,"Received %ld bytes but expected %d bytes for this \"writeSequential\" command.",len, 1);
                        break;
                    }
                    xTaskAbortDelay(modelTask()); // allow the uart port to be used again without delay.
                    xTaskNotify(modelTask(), fromUart, eSetValueWithoutOverwrite);
                    break;
                }
                len = uart_receive(buf, CMD_BUFF_SIZE, 0);
                if(len){
                    ESP_LOGW(TAG,"Received %ld unexpected bytes",len); 
                }
                break;
        }
    }
}

void flap_uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    xTaskCreate(flap_uart_task, "flap_uart_task", 6000, NULL, 10, &task);
    uart_api_init();
}

TaskHandle_t uartTask(){
    return task;
}

