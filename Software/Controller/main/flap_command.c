#include "flap_command.h"

// uint8_t get_cmd_data_len(uint8_t cmd)
// {
//     uint8_t data_len[] = {
//         0,
//         0,
//         66,
//         0,
//         0,
//         0,
//         0,
//         0,
//         0,
//         0,
//         0,
//         0,
//         0,
//         0,
//         0,
//         0
//     };
//     if((cmd & 0b00111111) >= sizeof(data_len))return 0;
//     return data_len[cmd & 0b00111111];
// }

int module_read_data_msg(char *buf, uint8_t data_len)
{
    if(buf){
        buf[0] = 0x80 + module_read_data;
        buf[1] = 0;
        buf[2] = 0;
        buf[3] = data_len;
    }
    return 4;
}
int module_goto_app_msg(char *buf, uint8_t extend)
{
    if(buf){
        buf[0] = (extend << 7) + module_goto_app;
    }
    return 1;
}
int module_goto_btl_msg(char *buf, uint8_t extend)
{
    if(buf){
        buf[0] = (extend << 7) + module_goto_btl;
    }
    return 1;
}

int module_get_config_msg(char *buf, uint8_t extend)
{
    if(buf){
       buf[0] = (extend << 7) + module_get_config;
    }
    return 1;
}
int module_get_char_msg(char *buf, uint8_t extend)
{
    if(buf){
        buf[0] = (extend << 7) + module_get_char;
    }
    return 1;
}

int module_get_charset_msg(char *buf, uint8_t extend)
{
    if(buf){
       buf[0] = (extend << 7) + module_get_charset;
    }
    return 1;
}

int module_set_offset_msg(char *buf, uint8_t extend, uint8_t data)
{
    if(buf){
        buf[0] = (extend << 7) + module_set_offset;
        buf[1] = data;
    }
    return 2;
}

int module_set_char_msg(char *buf, uint8_t extend, char *data)
{
    if(buf){
        buf[0] = (extend << 7) + module_set_char;
        memcpy(buf+1,data,4);
    }
    return 5;
}
int module_write_page_msg(char *buf, uint8_t extend,uint16_t addr, char *data)
{
    if(buf){
        buf[0] = (extend << 7) + module_write_page;
        buf[1] = (addr>>8) & 0xff;
        buf[2] = (addr>>0) & 0xff;
        memcpy(buf+3,data,64);
    }
    return 67;
}