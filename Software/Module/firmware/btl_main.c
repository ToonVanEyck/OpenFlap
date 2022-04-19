#include "config_bytes.h"

#include <xc.h>
#include <stdint.h>
#include <string.h>

#include "chain_comm.h"

#define _XTAL_FREQ 32000000

#define APP_START_ADDR      (0x800)
#define APP_END_ADDR        (0x1FFF - 96) // reserve 2 32 word pages for nvm data

#define INT_ADDR        APP_START_ADDR + 0x04
#define CS_ADDR         APP_END_ADDR - 1

extern void app_start() __at(APP_START_ADDR);
//extern void app__isr()  __at(INT_ADDR);

uint8_t watch_tx = 0;
uint8_t app_valid = 0;

void init_hardware(void)
{
    // IO
    ANSELA = 0x00;       // Enable digital drivers
    TRISA  = 0b00111000; // Set RA[5:3] as inputs and set others as outputs
    PORTA  = 0x00;       // Clear PORTA
    LATA   = 0x00;       // Clear Data Latch

    ANSELC = 0x00;       // Enable digital drivers
    INLVLC = 0b11000000; // Set TTL voltage levels
    TRISC  = 0b00111111; // Set RC[6:0] as inputs and set others as outputs
    PORTC  = 0x00;       // Clear PORTC
    LATC   = 0x00;       // Clear Data Latch
   
    // Peripheral mapping
    //uart
    RX1PPS = 0b000000100; // RX --> A4
    RA1PPS = 0x05;        // TX --> A1
    //pwm 
    RA2PPS = 0x01;        // CCP1 --> A2

    // UART
    BAUD1CON = 0x08;// BRG16;
    RC1STA = 0x90;//SPEN | CREN;
    TX1STA = 0x24;//TXEN | BRGH;

    // SP1BRGL = 68; //115200 baud
    // SP1BRGH = 0;

    SP1BRGL = 64; //9600 baud
    SP1BRGH = 3;

    // Timer 0 --> PID CLOCK
    T0CON1 = 0x44; // 1:1024 prescaler
    TMR0 = 0x00;
    T0CON0 = 0x90; // enable timer,16-bit, postscaler 1:1

    // Timer 1 --> UART TIMEOUT
    T1CLK = 0x01; // Fosc/4
    TMR1H = 00;
    TMR1L = 00;
    T1CON = 0x33; // 1:8 prescaler

    // Timer 2 --> PWM DRIVER
    T2PR = 249; // period -> 32kHz
    CCP1CON = 0x8C; // pwm mode
    CCPR1 = 0; // DC -> 0%
    T2HLT = 0x00;
    T2CLKCON = 0x01; // Fosc/4
    T2CON = 0x80;
}

void __interrupt() isr(void)
{

}

void write_page(uint8_t* rx_data,uint8_t* tx_data,cmd_info_t* cmd_info)
{
    if(cmd_info == NULL){
        clearAndWriteFlash(((uint16_t)rx_data[0]<< 8) | rx_data[1], rx_data+2);
    }else{
        // command info
        cmd_info->rx_data_len = 66;
        cmd_info->cmd = cmd_write_page;
        cmd_info->cmd_callback = write_page;
    }
}

void goto_app(uint8_t* rx_data,uint8_t* tx_data,cmd_info_t* cmd_info)
{
    if(cmd_info == NULL){
        if(validateCheckSum()) app_start();
    }else{
        // command info
        cmd_info->rx_data_len = 0;
        cmd_info->cmd = cmd_goto_app;
        cmd_info->cmd_callback = goto_app;
    }
}

void main(void)
{
    INTCONbits.GIE = 0;
    init_hardware();
    install_command(write_page);
    install_command(goto_app);
    while(1){
        CLRWDT();
        if(PIR1bits.RC1IF){
            TMR1H = 0;
            TMR1L = 0;
            chain_comm(comm_rx_data);
        }
        if(watch_tx && PIR1bits.TX1IF){
            watch_tx = 0;
            TMR1H = 0;
            TMR1L = 0;
            chain_comm(comm_tx_data);
        }
        if(PIR1bits.TMR1IF){
            PIR1bits.TMR1IF = 0;// clear interrupt flag
            TMR1H = 0;
            TMR1L = 0;
            chain_comm(comm_timeout);
        } 
    }
}
