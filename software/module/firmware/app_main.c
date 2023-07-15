#include "hardware_configuration.h"

#include <xc.h>
#include <stdint.h>
#include <string.h>

#include "chain_comm.h"
#include "version.h"

#define COL_END PORTAbits.RA5
#define MOTOR_DISABLE TRISAbits.TRISA2

uint8_t /*watch_tx,*/ char_index, do_a_loop, offset, vtrim;

char characterMap[4*NUM_CHARS] = {0};
const uint16_t speed[NUM_CHARS] = {       // distance to pwm conversion map
      0,  185,  185,  195,  195,  195,   \
    220,  220,  230,  230,  240,  240,   \
    260,  260,  270,  270,  280,  280,   \
    290,  290,  300,  300,  310,  310,   \
    320,  320,  330,  330,  340,  340,   \
    350,  350,  360,  360,  370,  370,   \
    380,  380,  390,  390,  400,  400,   \
    410,  410,  420,  420,  430,  430,   \
};

void set_default_characterMap(void){
    memset(characterMap,0,4*NUM_CHARS);

    NVMCON1bits.NVMREGS = 0;
    NVMADR = CHARSET_BASE_ADDR;
    NVMCON1bits.RD = 1;
    offset = ((uint8_t)(NVMDAT)) & 0x3F;
    vtrim = ((uint8_t)(NVMDAT >> 6)) & 0x3F;

    // combine 14-bit words to form characterMap
    for(uint16_t n=1,c=0,r=0,b=1;n/32 < 2 && c<sizeof(characterMap);){
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
        for(int i = 0; i<(r?2:1) && c<sizeof(characterMap);i++){
            characterMap[c] = (data >> (i?0:8)) & 0xFF;
            if(c%4 == 0 && characterMap[c]  == 0xFF) characterMap[c] = 0x20;
            if(c%4 == 0 && characterMap[c] == 'c'){characterMap[c] = 'C';b++;}
            if(c%4 == 0 && (characterMap[c] & 0xC0) == 0xC0) b++;
            if(c%4 == 0 && (characterMap[c] & 0xE0) == 0xE0) b++;
            if(c%4 == 0 && (characterMap[c] & 0xF0) == 0xF0) b++;
            if(c%4 + 1 == b){
                c+=(4-b);
                b = 1;
            }
            c++;
        }
    }
}

#define IR_OFF_TICKS 97//770
#define IR_ON_TICKS 3//20


uint8_t read_encoder(uint8_t is_idle)
{
    static unsigned pulse_cnt = IR_OFF_TICKS;
    static uint8_t enc_res = 0xff;
    static uint8_t enc_buffer[3] = {0};
    if(++pulse_cnt >= IR_OFF_TICKS){
        PORTAbits.RA0 = 1; // Enable IR LEDs
        if(pulse_cnt >= IR_OFF_TICKS + IR_ON_TICKS){
            pulse_cnt = 0;
            uint8_t enc_g = 0x3F;
            uint8_t enc_d = 0;
            enc_g ^= PORTCbits.RC3 << 5;
            enc_g ^= PORTCbits.RC2 << 4;
            enc_g ^= PORTCbits.RC4 << 3;
            enc_g ^= PORTCbits.RC1 << 2;
            enc_g ^= PORTCbits.RC5 << 1;
            enc_g ^= PORTCbits.RC0 << 0;
            PORTAbits.RA0 = 0; // Disable IR LEDs
            // convert gray code to decimal 
            for (enc_d = 0; enc_g; enc_g = enc_g >> 1) enc_d ^= enc_g;
            if(enc_d >(NUM_CHARS-1))enc_d =(NUM_CHARS-1);
            enc_d = (uint8_t)NUM_CHARS - enc_d - 1;
            // apply the encoder offset. 
            enc_d+= offset;
            if(enc_d >= NUM_CHARS) enc_d-= NUM_CHARS;
            // debounce readings
            enc_buffer[0] = is_idle? enc_buffer[1] : (uint8_t)enc_d;
            enc_buffer[1] = is_idle? enc_buffer[2] : (uint8_t)enc_d;
            enc_buffer[2] = (uint8_t)enc_d;
            if(enc_buffer[0] == enc_buffer[1] && enc_buffer[1] == enc_buffer[2]) enc_res = enc_d;
        }
    }
    return (uint8_t) enc_res;
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
            // INTCONbits.GIE = 0;
            NVMCON2 = 0x55;
            NVMCON2 = 0xAA;
            NVMCON1bits.WR = 1;
            NVMCON1bits.FREE = 0;
            NVMCON1bits.LWLO = 1;

            if(!(n >> 5)){
                NVMDAT = (uint16_t)offset + (uint16_t)(vtrim << 6);
                NVMCON2 = 0x55;
                NVMCON2 = 0xAA;
                NVMCON1bits.WR = 1;
                ++NVMADR;
                n++;
            }
        }
        uint16_t data = 0;
        if(c < 4*NUM_CHARS){
            if(r) data = ((uint16_t)characterMap[c-1] << (14-r)) & 0x3FFF;
            while(!(uint16_t)characterMap[c]){c++;}
            data += ((uint16_t)characterMap[c++] << (14 - r - 8));
            r = (r + 2) & 0x07;
            if(r){
                while(!(uint16_t)characterMap[c]){c++;}
                data += (uint16_t)characterMap[c++] >> r;
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
            // INTCONbits.GIE = 1;
        }
    }
}

asm("GLOBAL _app__isr");    // ensure ISR is not optimized out by compiler
__at(0x0808) void app__isr()// "real" isr routine is located in bootloader, that isr will call this function at 0x0808  
{

}

void do_command(uint8_t* data)
{
    switch(data[0]){
        case(reboot_command):
            RESET();     
        break;
        default:
            //no action
        break;
    }
}

void get_columnEnd(uint8_t* data)
{
    data[0] = COL_END;     
}

void get_version(uint8_t* data)
{
        data[0] = git_version_major;
        data[1] = git_version_minor;
        data[2] = git_version_patch;
        data[3] = git_version_tweak;
        memcpy((char*)data+4, git_version_hash, 7);
        data[11] = git_version_is_dirty;   
}

void set_character(uint8_t* data)
{
        // if(!strncmp("\0\0\0\n", (char *)data, 4)){ // do a full revolution
        //     do_a_loop = 1;
        // }
        // for(uint8_t i = 0; i < NUM_CHARS; i++){
        //     if(!strncmp(characterMap + i*4,(char*)data,4)){
        //         char_index = i;
        //         return;
        //     }
        // }
        char_index = data[0];  
}

void get_character(uint8_t* data)
{
    // strncpy((char*)data, characterMap + read_encoder(1)*4, 4);
    data[0] = read_encoder(1);
}

void get_characterMapSize(uint8_t* data)
{
    data[0] = NUM_CHARS;
}

void set_characterMap(uint8_t* data)
{
    memcpy(characterMap, (char*)data, 4*NUM_CHARS);
    store_config();
}

void get_characterMap(uint8_t* data)
{
    memcpy((char*)data, characterMap, 4*NUM_CHARS);
}

void set_offset(uint8_t* data)
{
    if(data[0] >= 0 && data[0] < NUM_CHARS){
        offset = data[0]; 
    }
    store_config();
}

void get_offset(uint8_t* data)
{
    data[0] = offset;
}

void set_vtrim(uint8_t* data)
{
    vtrim = data[0]; 
    store_config();
}

void get_vtrim(uint8_t* data)
{
    data[0] = vtrim;
}

uint16_t virtual_trim(uint16_t pwm_i)
{
    static uint16_t pwm_o = 0xFFFF;
    static uint16_t vtrim_count;
    if(pwm_o == 0xffff) pwm_o = pwm_i;
    if(pwm_i != pwm_o){
        vtrim_count++;  
        if(vtrim_count > ((uint16_t)vtrim*20)){
            vtrim_count = 0;
            pwm_o = pwm_i;
        }
    }
    return pwm_o;
} 

int motor_control(void)
{
    static uint16_t pwm = 0;

    int distance = (int)char_index - read_encoder(pwm == 0);
    if(do_a_loop){
        if(distance) do_a_loop = 0;
        distance = NUM_CHARS;
    }

    if(char_index == UNINITIALIZED) distance = 0;
    if(distance < 0) distance += NUM_CHARS;

    pwm = 0;
    if(distance >= 0 && distance < NUM_CHARS){
        pwm = speed[distance];
    }
    CCPR1 = virtual_trim(pwm);
    return distance;
}

void main(void)
{
    // init vars
    char_index = UNINITIALIZED;
    do_a_loop = 0;
    offset = 0;
    vtrim = 0;
    // init hw and peripherals
    init_hardware();
    // enable interrupts
    // INTCONbits.GIE = 1;
    // INTCONbits.PEIE = 1;
    // load default characterMap from NVM
    set_default_characterMap();
    // install callbacks for uart chain-comm commands
    addPropertyHandler(firmware_property,get_version,NULL);
    addPropertyHandler(command_property,NULL,do_command);
    addPropertyHandler(columnEnd_property,get_columnEnd,NULL);
    addPropertyHandler(character_property,get_character,set_character);
    addPropertyHandler(characterMapSize_property,get_characterMapSize,NULL);
    addPropertyHandler(characterMap_property,get_characterMap,set_characterMap);
    addPropertyHandler(offset_property,get_offset,set_offset);
    addPropertyHandler(vtrim_property,get_vtrim,set_vtrim);

    uint32_t idle_timer = 0; // timeout counter
    int distance_to_rotate = 0;

    TX_BYTE(0x00); //send ack to indicate successful boot 
    TX_WAIT_DONE;
    // main loop
    while(1){
        CLRWDT();
        chainCommRun(&idle_timer);
        if(idle_timer >= 10000 || distance_to_rotate) idle_timer = 0; // 1 seconds 
        if(idle_timer < 2500){  // 250ms -> Not idle
            distance_to_rotate = motor_control();
        }else{          // Idle
            PORTAbits.RA0 = 0; // disable IR led when idle
        }
    }
}