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

#ifdef IS_BTL
    #define CMD_SIZE (module_goto_app +1)
#else
    #define CMD_SIZE (end_of_command +1)
#endif


extern uint8_t watch_tx;

/* This function checks if the communication protocol needs or received an update.
Increments the idle_timeout or resets it if communication occurs. */
// void chain_comm_loop(uint32_t *idle_timeout); 
/* Adds callback functions to the protocol */
// void install_command(void (*cmd_callback)(uint8_t*,uint8_t*,cmd_info_t*));
/* This function handles the communication protocol and call appropriate callback functions. */
// void chain_comm(uint8_t new_comm_data);

// new shit

#pragma pack(1)

typedef enum{
    dataAvailable,
    transmitComplete,
    communicationTimeout,
}chainCommEvent_t;

typedef enum{
    receiveHeader = 0,
    indexModules,
    receiveData,
    passthrough,
    transmitData,
    receiveAcknowledge,
    errorIgnoreData,
    waitForAcknowledge,
}chainCommState_t;

typedef struct{
    chainCommState_t state;
    uint16_t index;
    chainCommHeader_t header;
    uint8_t cnt;
    uint8_t data[CHAIN_COM_MAX_LEN];
}chainCommCtx_t;

typedef void (*property_callback)(uint8_t* buf);

typedef struct{
    property_callback get;
    property_callback set;
}propertyHandler_t;

static propertyHandler_t propertyHandlers[end_of_properties] = {0};

void addPropertyHandler(moduleProperty_t property, property_callback get, property_callback set);
void chainCommRun(uint32_t *idle_timeout); 
void chainComm(chainCommEvent_t event);



#endif