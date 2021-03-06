#ifndef CHAIN_COMM_H
#define CHAIN_COMM_H

#include <xc.h>
#include <stdint.h>
#include <string.h>
#include "chain_comm_abi.h"
#include "flash.h"

#define _XTAL_FREQ 32000000

#ifndef DEBUG_WIGGLE
    #define DEBUG_WIGGLE() PORTAbits.RA0 ^= 1
#endif 
#ifndef DEBUG_PRINT
    #define DEBUG_PRINT(...) ;
#endif
#ifndef RX_BYTE // receive a byte
    #define RX_BYTE(_b) _b = RC1REG
#endif 
#ifndef RX_DONE // receive is done (interrupt flag)
    #define RX_DONE PIR1bits.RC1IF
#endif 
#ifndef TX_BYTE // transmit a byte
    #define TX_BYTE(_b) while(!PIR1bits.TX1IF); watch_tx = 1; TX1REG = _b
#endif
#ifndef TX_DONE // transmit is done (interrupt flag)
    #define TX_DONE PIR1bits.TX1IF
#endif
#ifndef TX_WAIT_DONE // wait until previous transmit has finished is done
    #define TX_WAIT_DONE do{NOP();}while(!(PIR1bits.TX1IF && TX1STAbits.TRMT))
#endif
#ifndef CLOCK_GUARD // wait until timer is triggered 
    #define CLOCK_GUARD while(!PIR0bits.TMR0IF);PIR0bits.TMR0IF = 0
#endif

#define CMD_EXTEND 0x80
#define CMD_CMD 0x0F

#ifdef IS_BTL
    #define CMD_SIZE (module_goto_app +1)
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
    module_command_t cmd;
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

/* This function checks if the communication protocol needs or received an update.
Increments the idle_timeout or resets it if communication occurs. */
void chain_comm_loop(uint32_t *idle_timeout); 
/* Adds callback functions to the protocol */
void install_command(void (*cmd_callback)(uint8_t*,uint8_t*,cmd_info_t*));
/* This function handles the communication protocol and call appropriate callback functions. */
void chain_comm(uint8_t new_comm_data);

#endif