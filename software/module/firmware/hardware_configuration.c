#include "hardware_configuration.h"

void init_hardware(void)
{
    // IO
    ANSELA = 0x00;      // Enable digital drivers
    TRISA = 0b00111000; // Set RA[5:3] as inputs and set others as outputs
    PORTA = 0x00;       // Clear PORTA
    LATA = 0x00;        // Clear Data Latch

    ANSELC = 0x00;       // Enable digital drivers
    INLVLC = 0b11000000; // Set TTL voltage levels
    TRISC = 0b00111111;  // Set RC[6:0] as inputs and set others as outputs
    PORTC = 0x00;        // Clear PORTC
    LATC = 0x00;         // Clear Data Latch

    // Peripheral mapping
    // uart
    RX1PPS = 0b000000100; // RX --> A4
    RA1PPS = 0x05;        // TX --> A1
    // pwm
    RA2PPS = 0x01; // CCP1 --> A2
    RA0PPS = 0x02; // CCP2 --> A0 pwm 2 to drive ir led?
    // UART
    BAUD1CON = 0x08; // BRG16;
    RC1STA = 0x90;   // SPEN | CREN;
    TX1STA = 0x24;   // TXEN | BRGH;

    SP1BRGL = 68; // 115200 baud
    SP1BRGH = 0;

    // Timer 0 --> 10kHz clock
    T0CON1 = 0x45; // 1:32 prescaler
    TMR0 = 0x00;
    TMR0H = 24;
    TMR0L = 0;
    T0CON0 = 0x80; // enable timer, 8-bit, postscaler 1:1
    PIE0bits.TMR0IE = 1;

    // Timer 2 --> PWM DRIVER
    T2PR = 249;     // period -> 32kHz
    CCP1CON = 0x8C; // pwm mode
    CCPR1 = 0;      // DC -> 0%
    CCP2CON = 0x8C; // pwm mode
    CCPR2 = 0;      // DC -> 0%
    T2HLT = 0x00;
    T2CLKCON = 0x01; // Fosc/4
    T2CON = 0x80;

    // IR PWM
}
