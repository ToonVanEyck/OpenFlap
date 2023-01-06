#ifndef FLAP_COMMAND_H
#define FLAP_COMMAND_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "chain_comm_abi.h"


#define CMD_COMM_BUF_LEN 2048
#define EXTEND (0x80)

// inline void cmd_send_read_data(uint8_t size){
//     char cmd_uart_buf[4]={0};
//     cmd_uart_buf[0] = EXTEND + module_read_data;
//     cmd_uart_buf[3] = size;
//     flap_uart_send_data(cmd_uart_buf,4); 
// }

// inline void cmd_send_get_config(){
//     char cmd_uart_buf[1]={0};
//     cmd_uart_buf[0] = EXTEND + module_get_config;
//     flap_uart_send_data(cmd_uart_buf,1); 
//     cmd_send_read_data(4);
// }

// inline void cmd_send_get_fw_version(){
//     char cmd_uart_buf[1]={0};
//     cmd_uart_buf[0] = EXTEND + module_get_fw_version;
//     flap_uart_send_data(cmd_uart_buf,1); 
//     cmd_send_read_data(12);
// }

// inline void cmd_send_get_char(){
//     char cmd_uart_buf[1]={0};
//     cmd_uart_buf[0] = EXTEND + module_get_char;
//     flap_uart_send_data(cmd_uart_buf,1); 
//     cmd_send_read_data(4);
// }

// inline void cmd_send_get_charset(){
//     char cmd_uart_buf[1]={0};
//     cmd_uart_buf[0] = EXTEND + module_get_charset;
//     flap_uart_send_data(cmd_uart_buf,1); 
//     cmd_send_read_data(4*48);
// }

// inline void cmd_send_get_offset(){
//     char cmd_uart_buf[1]={0};
//     cmd_uart_buf[0] = EXTEND + module_get_offset;
//     flap_uart_send_data(cmd_uart_buf,1); 
//     cmd_send_read_data(1);
// }

// inline void cmd_send_get_vtrim(){
//     char cmd_uart_buf[1]={0};
//     cmd_uart_buf[0] = EXTEND + module_get_vtrim;
//     flap_uart_send_data(cmd_uart_buf,1); 
//     cmd_send_read_data(1);
// }

typedef struct{
    size_t data_len;
    size_t data_offset;
    size_t total_data_len;
    char data[CMD_COMM_BUF_LEN];
}controller_queue_data_t;

#endif