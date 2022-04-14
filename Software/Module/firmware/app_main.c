#include "config_bytes.h"

#include <xc.h>
#include <stdint.h>
#include <string.h>

#include "chain_comm.h"

#define _XTAL_FREQ 32000000

#define COL_END PORTAbits.RA5
#define MOTOR_DISABLE TRISAbits.TRISA2

#define REV_CNT_BASE_ADDR (0x1FE0)
#define REV_CNT_MULT_ADDR (REV_CNT_BASE_ADDR + 31)

#define CHARSET_BASE_ADDR (0x1FA0)

#define NUM_CHARS 48

uint8_t watch_tx = 0;
uint8_t char_index = 0;
uint8_t rev_add = 0;
uint32_t rev_cnt = 0;
uint8_t offset = 0;

char charset[4*NUM_CHARS] = {0};
const uint16_t speed[NUM_CHARS] = {       // distance to pwm conversion map
      0,  165,  165,  175,  175,  175,   \
    200,  200,  210,  210,  220,  220,   \
    240,  240,  250,  250,  260,  260,   \
    270,  270,  280,  280,  290,  290,   \
    300,  300,  310,  310,  320,  320,   \
    330,  330,  340,  340,  350,  350,   \
    360,  360,  370,  370,  380,  380,   \
    390,  390,  400,  400,  410,  410,   \
};

void set_default_charset(void){
    memset(charset,0,4*NUM_CHARS);

    NVMCON1bits.NVMREGS = 0;
    NVMADR = CHARSET_BASE_ADDR;
    NVMCON1bits.RD = 1;
    offset = ((uint8_t)(NVMDAT)) & 0xFF;

    // combine 14-bit words to form charset
    for(uint16_t n=1,c=0,r=0,b=1;n/32 < 2 && c<sizeof(charset);){
        NVMCON1bits.NVMREGS = 0;
        uint16_t data = 0;
        NVMADR = n + CHARSET_BASE_ADDR;
        NVMCON1bits.RD = 1;
        data = NVMDAT << (2 + r);
        r = (r + 2) & 0x07; 
        n++;
        NVMADR = n + CHARSET_BASE_ADDR;
        NVMCON1bits.RD = 1;
        data += NVMDAT >> (14 - r);
        for(int i = 0; i<(r?2:1) && c<sizeof(charset);i++){
            charset[c] = (data >> (i?0:8)) & 0xFF;
            if(c%4 == 0 && charset[c]  == 0xFF) charset[c] = 0x20;
            if(c%4 == 0 && charset[c] == 'c'){charset[c] = 'C';b++;}
            if(c%4 == 0 && (charset[c] & 0xC0) == 0xC0) b++;
            if(c%4 == 0 && (charset[c] & 0xE0) == 0xE0) b++;
            if(c%4 == 0 && (charset[c] & 0xF0) == 0xF0) b++;
            if(c%4 + 1 == b){
                c+=(4-b);
                b = 1;
            }
            c++;
        }
    }
}

#define IR_OFF_TICKS 770
#define IR_ON_TICKS 20

void init_encoder(void){
    for(int i = 0; i < (IR_ON_TICKS + IR_OFF_TICKS);i++){
        read_encoder();
    }
    char_index = read_encoder();
}


int read_encoder(void)
{
    static unsigned pulse_cnt = IR_OFF_TICKS;
    static int encoder_dec = 0;
    if(++pulse_cnt >= IR_OFF_TICKS){
        PORTAbits.RA0 = 1; // Enable IR LEDs
        if(pulse_cnt >= IR_OFF_TICKS + IR_ON_TICKS){
            int encoder_grey = 0x3F;
            encoder_grey ^= PORTCbits.RC3 << 5;
            encoder_grey ^= PORTCbits.RC2 << 4;
            encoder_grey ^= PORTCbits.RC4 << 3;
            encoder_grey ^= PORTCbits.RC1 << 2;
            encoder_grey ^= PORTCbits.RC5 << 1;
            encoder_grey ^= PORTCbits.RC0 << 0;
            PORTAbits.RA0 = 0; // Disable IR LEDs
            int prev_encoder_dec = encoder_dec;
            for (encoder_dec = 0; encoder_grey; encoder_grey = encoder_grey >> 1)
                encoder_dec ^= encoder_grey;
            encoder_dec = NUM_CHARS - encoder_dec - 1;
            pulse_cnt = 0;
            if(prev_encoder_dec - 3 > encoder_dec) rev_add++; // The encoder has roled over, a revolution was completed.
        }
    }
    int tmp_encoder_val = encoder_dec + (int)offset;
    if(tmp_encoder_val >= NUM_CHARS) tmp_encoder_val -= NUM_CHARS;
    return tmp_encoder_val;
}

void store_config(void)
{
    for(uint16_t c=0,n=0,r=0;n/32 < 2;){
        if(!(n%32)){ // erase page
            NVMCON1bits.WRERR = 0;
            NVMADR = n + CHARSET_BASE_ADDR;
            NVMCON1bits.NVMREGS = 0;
            NVMCON1bits.FREE = 1;
            NVMCON1bits.WREN = 1;
            NVMCON2 = 0x55;
            NVMCON2 = 0xAA;
            NVMCON1bits.WR = 1;
            NVMCON1bits.FREE = 0;
            NVMCON1bits.LWLO = 1;

            if(!(n >> 5)){
                NVMDAT = offset;
                NVMCON2 = 0x55;
                NVMCON2 = 0xAA;
                NVMCON1bits.WR = 1;
                ++NVMADR;
                n++;
            }
        }
        uint16_t data = 0;
        if(c < 4*NUM_CHARS){
            if(r) data = ((uint16_t)charset[c-1] << (14-r)) & 0x3FFF;
            while(!(uint16_t)charset[c]){c++;}
            data += ((uint16_t)charset[c++] << (14 - r - 8));
            r = (r + 2) & 0x07;
            if(r){
                while(!(uint16_t)charset[c]){c++;}
                data += (uint16_t)charset[c++] >> r;
            }
        }else{
            data = 0x3FFF;
        }
        NVMDAT = data;
        data = 0;
        if((n++%32) < 31){
            NVMCON2 = 0x55;
            NVMCON2 = 0xAA;
            NVMCON1bits.WR = 1;
            ++NVMADR;
        }else{
            NVMCON1bits.LWLO = 0;
            NVMCON2 = 0x55;
            NVMCON2 = 0xAA;
            NVMCON1bits.WR = 1;
            NVMCON1bits.WREN = 0;
        }
    }
}

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
    CCPR1 = 100; // DC -> 10%
    T2HLT = 0x00;
    T2CLKCON = 0x01; // Fosc/4
    T2CON = 0x80;
}

void __interrupt() isr(void)
{

}

void goto_btl(uint8_t* rx_data,uint8_t* tx_data,cmd_info_t* cmd_info)
{
    if(cmd_info == NULL){
        __delay_ms(2);
        RESET();
    }else{
        // command info
        cmd_info->rx_data_len = 0;
        cmd_info->cmd = cmd_goto_btl;
        cmd_info->cmd_callback = goto_btl;
    }
}

void get_config(uint8_t* rx_data,uint8_t* tx_data,cmd_info_t* cmd_info)
{
    if(cmd_info == NULL){
        tx_data[0] = COL_END;
    }else{
        // command info
        cmd_info->rx_data_len = 0;
        cmd_info->cmd = cmd_get_config;
        cmd_info->cmd_callback = get_config;
    }
}

void get_rev_cnt(uint8_t* rx_data,uint8_t* tx_data,cmd_info_t* cmd_info)
{
    if(cmd_info == NULL){
        tx_data[0] = rev_cnt >>  0 & 0xFF; 
        tx_data[1] = rev_cnt >>  8 & 0xFF; 
        tx_data[2] = rev_cnt >> 16 & 0xFF; 
        tx_data[3] = rev_cnt >> 24 & 0xFF; 
    }else{
        // command info
        cmd_info->rx_data_len = 0;
        cmd_info->cmd = cmd_get_rev_cnt;
        cmd_info->cmd_callback = get_rev_cnt;
    }
}

void set_char(uint8_t* rx_data,uint8_t* tx_data,cmd_info_t* cmd_info)
{
    if(cmd_info == NULL){
        for(uint8_t i = 0; i < NUM_CHARS; i++){
            if(!strncmp(charset + i*4,(char*)rx_data,4)){
                char_index = i;
                return;
            }
        }
        char_index = 0;
    }else{
        // command info
        cmd_info->rx_data_len = 4;
        cmd_info->cmd = cmd_set_char;
        cmd_info->cmd_callback = set_char;
    }
}

void get_char(uint8_t* rx_data,uint8_t* tx_data,cmd_info_t* cmd_info)
{
    if(cmd_info == NULL){
        strncpy((char*)tx_data, charset + read_encoder()*4, 4);
    }else{
        // command info
        cmd_info->rx_data_len = 0;
        cmd_info->cmd = cmd_get_char;
        cmd_info->cmd_callback = get_char;
    }
}

void set_charset(uint8_t* rx_data,uint8_t* tx_data,cmd_info_t* cmd_info)
{
    if(cmd_info == NULL){
        memcpy(charset, (char*)rx_data, 4*NUM_CHARS);
        store_config();
    }else{
        // command info
        cmd_info->rx_data_len = 4*NUM_CHARS;
        cmd_info->cmd = cmd_set_charset;
        cmd_info->cmd_callback = set_charset;
    }
}

void get_charset(uint8_t* rx_data,uint8_t* tx_data,cmd_info_t* cmd_info)
{
    if(cmd_info == NULL){
        memcpy((char*)tx_data, charset, 4*NUM_CHARS);
    }else{
        // command info
        cmd_info->rx_data_len = 0;
        cmd_info->cmd = cmd_get_charset;
        cmd_info->cmd_callback = get_charset;
    }
}

void set_offset(uint8_t* rx_data,uint8_t* tx_data,cmd_info_t* cmd_info)
{
    if(cmd_info == NULL){
        offset = rx_data[0];
        store_config();
    }else{
        // command info
        cmd_info->rx_data_len = 1;
        cmd_info->cmd = cmd_set_offset;
        cmd_info->cmd_callback = set_offset;
    }
}

int motor_control(void)
{
    int distance = (int)char_index - read_encoder();
    if(distance < 0) distance += NUM_CHARS;
    unsigned short pwm = 0;
    if(distance >= 0 && distance < NUM_CHARS){
        pwm = speed[distance];
    }
    CCPR1 = pwm;
    return distance;
}

uint32_t calc_rev_cnt(){
    NVMCON1bits.NVMREGS = 0;
    uint16_t data = 0;
    uint16_t cnt = 0;
    for(int word_i = 0; word_i < 31;word_i++){
        CLRWDT();
        NVMADR = REV_CNT_BASE_ADDR;
        NVMCON1bits.RD = 1;
        data = NVMDAT;
        for(uint16_t bit_i = 0x3FFF; data < bit_i ; bit_i = (bit_i<<1)&0x3fff){
            cnt++;
        }
        if(data > 0) break;
    }
    NVMADR = REV_CNT_MULT_ADDR;
    NVMCON1bits.RD = 1;
    data = 0x3FFF - NVMDAT;
    return ((uint32_t)cnt + (uint32_t)data * 434);
}

void increment_rev_cnt(){
    if(!rev_add) return;   // only execute 
    uint16_t rev_multiplier = (uint16_t)((rev_cnt + rev_add) / (uint16_t)434);  // number of total filled pages
    uint16_t rec_remainder = (rev_cnt + rev_add) % (uint16_t)434;   // number of addition bits set

    NVMCON1bits.WRERR = 0;
    NVMADR = REV_CNT_BASE_ADDR;
    if(rev_cnt / 434 < rev_multiplier){
        // clear page
        NVMCON1bits.NVMREGS = 0;
        NVMCON1bits.FREE = 1;
        NVMCON1bits.WREN = 1;
        // Unlock Sequence
        NVMCON2 = 0x55;
        NVMCON2 = 0xAA;
        NVMCON1bits.WR = 1;
    }

    NVMCON1bits.FREE = 0;
    NVMCON1bits.LWLO = 1;
    NVMCON1bits.WREN = 1;
    for(int i = 0;i<31;i++){
        uint16_t  word = 0x3FFF;
        for(int j = 0; j < 14;j++){
            if(rec_remainder){
                word = word << 1 & 0x3FFF;
                rec_remainder--;
            }
        }
        NVMDAT = word;
        NVMCON2 = 0x55;
        NVMCON2 = 0xAA;
        NVMCON1bits.WR = 1;
        ++NVMADR;
    }
    NVMDAT = (0X3FFF - rev_multiplier) & 0x3FFF;
    NVMCON1bits.LWLO = 0;
    NVMCON2 = 0x55;
    NVMCON2 = 0xAA;
    NVMCON1bits.WR = 1;
    NVMCON1bits.WREN = 0;

    rev_cnt += rev_add;
    rev_add = 0;
}

void main(void)
{
    INTCONbits.GIE = 0;
    init_hardware();
    set_default_charset();
    install_command(goto_btl);
    install_command(get_config);
    install_command(set_char);
    install_command(get_char);
    install_command(get_rev_cnt);
    install_command(set_charset);
    install_command(get_charset);
    install_command(set_offset);

    rev_cnt = calc_rev_cnt();
    init_encoder();

    uint8_t toc = 0; // timeout counter
    int distance_to_rotate = 0;
    while(1){
        CLRWDT();
        if(PIR1bits.RC1IF){
            toc = 0;
            TMR1H = 0;
            TMR1L = 0;
            chain_comm(comm_rx_data);
        }
        if(watch_tx && PIR1bits.TX1IF){
            toc = 0;
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
            if(toc++ > 32)toc = 0;//*/toc++;
        } 
        if(distance_to_rotate) toc = 0;
        if(toc < 8){   // Not idle
            distance_to_rotate = motor_control();
        }else{          // Idle
            PORTAbits.RA0 = 0; // disable IR led when idle
            increment_rev_cnt();
        }
    }
}