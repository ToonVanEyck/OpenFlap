#ifndef CHAIN_COMM_ABI_H
#define CHAIN_COMM_ABI_H

// set command manipulation  (for vtrim and offset)
#define SET_CMD_VALUE(_b)  ((_b) & 0x3F) 

#define SET_CMD_IS_ABS(_b) (((_b) & 0xC0) == 0x00) 
#define SET_CMD_MODE_ABS(_b) ((_b) & 0x3F)

#define SET_CMD_IS_INC(_b) ((_b) & 0x80) 
#define SET_CMD_MODE_INC(_b) (((_b) & 0x3F) + 0x80) 

#define SET_CMD_IS_DEC(_b) ((_b) & 0x40) 
#define SET_CMD_MODE_DEC(_b) (((_b) & 0x3F) + 0x40) 

typedef enum {ABS, INC, DEC} cmd_set_int_mode_t;

typedef enum{
    module_do_nothing,
    module_read_data,
    module_write_page,
    module_goto_app,  // Last command of bootloader supported commands

    module_goto_btl,
    module_get_config,
    module_get_fw_version,
    module_get_hw_id,
    module_set_char,
    module_get_char,
    module_set_charset,
    module_get_charset,
    module_set_offset,
    module_get_offset,
    module_set_vtrim,
    module_get_vtrim,

    end_of_command //DONT INSERT COMMANDS AFTER THIS ONE
}module_command_t;

#endif 