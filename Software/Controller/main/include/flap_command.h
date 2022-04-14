#ifndef FLAP_COMMAND_H
#define FLAP_COMMAND_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


#define CMD_COMM_BUF_LEN 2048

typedef enum{
    controller_idle,
    controller_rx_data,
    controller_controller_firmware,
    controller_module_firmware,
    controller_get_dimensions,
    controller_set_message,
    controller_get_message,
    controller_set_charset,
    controller_get_charset,
    controller_set_offset,
    controller_goto_app
}controller_command_t;

typedef enum{
    module_do_nothing,
    module_read_data,
    module_write_page,
    module_goto_app,  // Last command of bootloader supported commands
    module_goto_btl,
    module_get_config,
    module_get_fw_vesion,
    module_get_hw_id,
    module_get_rev_cnt,
    module_set_char,
    module_get_char,
    module_set_charset,
    module_get_charset,
    module_set_offset,
    module_get_flap,

    end_of_command //DONT INSERT COMMANDS AFTER THIS ONE
}module_command_t;

typedef struct{
    controller_command_t cmd;
    size_t data_len;
    size_t data_offset;
    size_t total_data_len;
    char data[CMD_COMM_BUF_LEN];
}controller_queue_data_t;

#define WEB_SET_DIMENSIONS_TEMPLATE(_col,_row) "{\"command\":\"get_display_dimensions_res\",\"data\":{\"height\":%d,\"width\":%d}}",(_row),(_col)
#define WEB_SET_CHAR_TEMPLATE(_message) "{\"command\":\"get_display_message_res\",\"data\":\"%s\"}",(_message)
#define WEB_SET_CHARSET_TEMPLATE(_id,_message) "{\"command\":\"get_display_charset_res\",\"flap_id\":%d,\"data\":[\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"]}",(_id),_message+(0*4),_message+(1*4),_message+(2*4),_message+(3*4),_message+(4*4),_message+(5*4),_message+(6*4),_message+(7*4),_message+(8*4),_message+(9*4),_message+(10*4),_message+(11*4),_message+(12*4),_message+(13*4),_message+(14*4),_message+(15*4),_message+(16*4),_message+(17*4),_message+(18*4),_message+(19*4),_message+(20*4),_message+(21*4),_message+(22*4),_message+(23*4),_message+(24*4),_message+(25*4),_message+(26*4),_message+(27*4),_message+(28*4),_message+(29*4),_message+(30*4),_message+(31*4),_message+(32*4),_message+(33*4),_message+(34*4),_message+(35*4),_message+(36*4),_message+(37*4),_message+(38*4),_message+(39*4),_message+(40*4),_message+(41*4),_message+(42*4),_message+(43*4),_message+(44*4),_message+(45*4),_message+(46*4),_message+(47*4)
#define WEB_DEBUG_LOG_TEMPLATE_BEGIN "{\"command\":\"debug_log_uart\",\"data\":\""
#define WEB_DEBUG_LOG_TEMPLATE_END "\"}"

uint8_t get_cmd_data_len(uint8_t cmd);

int module_read_data_msg(char *buf, uint8_t data_len);
int module_goto_app_msg(char *buf, uint8_t extend);
int module_goto_btl_msg(char *buf, uint8_t extend);

int module_get_config_msg(char *buf, uint8_t extend);
int module_get_char_msg(char *buf, uint8_t extend);

int module_get_charset_msg(char *buf, uint8_t extend);
int module_set_offset_msg(char *buf, uint8_t extend, uint8_t data);

int module_set_char_msg(char *buf, uint8_t extend, char *data);
int module_write_page_msg(char *buf, uint8_t extend,uint16_t addr, char *data);


#endif