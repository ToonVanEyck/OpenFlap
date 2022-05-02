#ifndef FLAP_COMMAND_H
#define FLAP_COMMAND_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


#define CMD_COMM_BUF_LEN 2048
#define EXTEND (0x80)

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
    size_t data_len;
    size_t data_offset;
    size_t total_data_len;
    char data[CMD_COMM_BUF_LEN];
}controller_queue_data_t;

#endif