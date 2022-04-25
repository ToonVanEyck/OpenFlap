#ifndef CHAIN_COMM_H
#define CHAIN_COMM_H

#include <xc.h>
#include <stdint.h>
#include <string.h>
#include "flash.h"

#define _XTAL_FREQ 32000000

#ifndef DEBUG_WIGGLE
    #define DEBUG_WIGGLE() PORTAbits.RA0 ^= 1
#endif 
#ifndef DEBUG_PRINT
    #define DEBUG_PRINT(...) ;
#endif
#ifndef RX_BYTE
    #define RX_BYTE(_b) _b = RC1REG
#endif 
#ifndef TX_BYTE
    #define TX_BYTE(_b) while(!PIR1bits.TX1IF); watch_tx = 1; TX1REG = _b
#endif
#ifndef TX_DONE
    #define TX_DONE do{NOP();}while(!(PIR1bits.TX1IF && TX1STAbits.TRMT))
#endif

#define CMD_EXTEND 0x80
#define CMD_CMD 0x0F

typedef enum{
    cmd_do_nothing,
    cmd_read_data,
    cmd_write_page,
    cmd_goto_app,  // Last command of bootloader supported commands

    cmd_goto_btl,
    cmd_get_config,
    cmd_get_fw_version,
    cmd_get_hw_id,
    cmd_get_rev_cnt,
    cmd_set_char,
    cmd_get_char,
    cmd_set_charset,
    cmd_get_charset,
    cmd_set_offset,

    end_of_command //DONT INSERT COMMANDS AFTER THIS ONE
}command_t;

#ifdef IS_BTL
    #define CMD_SIZE (cmd_goto_app +1)
#else
    #define CMD_SIZE (end_of_command +1)
#endif

typedef enum{
    comm_state_command = 0,
    comm_state_data,
    comm_state_passthrough,
    comm_state_transmit,
}comm_state_t;

typedef enum{
    comm_rx_data,
    comm_tx_data,
    comm_timeout,
}new_comm_data_t;

typedef struct cmd_info_t{
    uint8_t rx_data_len;
    command_t cmd;
    void (*cmd_callback)(uint8_t*,uint8_t*,struct cmd_info_t*);
}cmd_info_t;

#pragma pack(1)
typedef struct{
    comm_state_t state;
    uint8_t rx_data_cnt;
    uint8_t tx_data_cnt;
    uint16_t tx_node_cnt;
    uint8_t carry;
    uint8_t command; // 1 extend cmd bit  3 reserved bits 4 cmd bits
    uint8_t rx_data[256];
    uint8_t tx_data[256];
}comm_ctx_t;

extern uint8_t watch_tx;

void install_command(void (*cmd_callback)(uint8_t*,uint8_t*,cmd_info_t*));
void chain_comm(uint8_t new_comm_data);

#endif