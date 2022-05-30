#include "hardware_configuration.h"

#include <xc.h>
#include <stdint.h>
#include <string.h>

#include "chain_comm.h"
#include "version.h"

#define COL_END PORTAbits.RA5
#define MOTOR_DISABLE TRISAbits.TRISA2

uint8_t /*watch_tx,*/ char_index, rev_add, offset, vtrim;
uint32_t rev_cnt;

char charset[4*NUM_CHARS] = {0};
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

void set_default_charset(void){
    memset(charset,0,4*NUM_CHARS);

    NVMCON1bits.NVMREGS = 0;
    NVMADR = CHARSET_BASE_ADDR;
    NVMCON1bits.RD = 1;
    offset = ((uint8_t)(NVMDAT)) & 0x3F;
    vtrim = ((uint8_t)(NVMDAT >> 6)) & 0x3F;

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

#define IR_OFF_TICKS 97//770
#define IR_ON_TICKS 3//20


uint8_t read_encoder(uint8_t is_idle)
{
    static unsigned pulse_cnt = IR_OFF_TICKS;
    static uint8_t enc_res = 0xff;
    static uint8_t prev_rev_cnt_state = 0;
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
            // check if the revolution counter needs to be incremented
            if(prev_rev_cnt_state){
                if(PORTCbits.RC3 && ! PORTCbits.RC2){
                    rev_add++; 
                    prev_rev_cnt_state = 0;
                }
            }else{
                if(PORTCbits.RC2 && ! PORTCbits.RC3) prev_rev_cnt_state = 1; 
            }
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
            // INTCONbits.GIE = 1;
        }
    }
}

asm("GLOBAL _app__isr");    // ensure ISR is not optimized out by compiler
__at(0x0808) void app__isr()// "real" isr routine is located in bootloader, that isr will call this function at 0x0808  
{

}

void do_nothing(uint8_t* rx_data,uint8_t* tx_data,cmd_info_t* cmd_info)
{
    if(cmd_info == NULL){
    }else{
        // command info
        cmd_info->rx_data_len = 0;
        cmd_info->cmd = module_do_nothing;
        cmd_info->cmd_callback = do_nothing;
    }
}

void goto_btl(uint8_t* rx_data,uint8_t* tx_data,cmd_info_t* cmd_info)
{
    if(cmd_info == NULL){
        RESET();
    }else{
        // command info
        cmd_info->rx_data_len = 0;
        cmd_info->cmd = module_goto_btl;
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
        cmd_info->cmd = module_get_config;
        cmd_info->cmd_callback = get_config;
    }
}

void get_fw_version(uint8_t* rx_data,uint8_t* tx_data,cmd_info_t* cmd_info)
{
    if(cmd_info == NULL){
        tx_data[0] = git_version_major;
        tx_data[1] = git_version_minor;
        tx_data[2] = git_version_patch;
        tx_data[3] = git_version_commits_after_tag;
        memcpy((char*)tx_data+4, git_version_hash, 7);
        tx_data[11] = git_version_is_dirty;
    }else{
        // command info
        cmd_info->rx_data_len = 0;
        cmd_info->cmd = module_get_fw_version;
        cmd_info->cmd_callback = get_fw_version;
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
        cmd_info->cmd = module_get_rev_cnt;
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
        cmd_info->cmd = module_set_char;
        cmd_info->cmd_callback = set_char;
    }
}

void get_char(uint8_t* rx_data,uint8_t* tx_data,cmd_info_t* cmd_info)
{
    if(cmd_info == NULL){
        strncpy((char*)tx_data, charset + read_encoder(1)*4, 4);
    }else{
        // command info
        cmd_info->rx_data_len = 0;
        cmd_info->cmd = module_get_char;
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
        cmd_info->cmd = module_set_charset;
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
        cmd_info->cmd = module_get_charset;
        cmd_info->cmd_callback = get_charset;
    }
}

void set_offset(uint8_t* rx_data,uint8_t* tx_data,cmd_info_t* cmd_info)
{
    if(cmd_info == NULL){
        if(SET_CMD_VALUE(rx_data[0]) >= 0 && SET_CMD_VALUE(rx_data[0]) < NUM_CHARS){
            if(SET_CMD_IS_ABS(rx_data[0])){
                offset = SET_CMD_VALUE(rx_data[0]);
            }else if(SET_CMD_IS_INC(rx_data[0])){
                offset += SET_CMD_VALUE(rx_data[0]);
                if(offset >= NUM_CHARS) offset -= NUM_CHARS;
            }else if(SET_CMD_IS_DEC(rx_data[0])){
                if(offset < SET_CMD_VALUE(rx_data[0])) offset += NUM_CHARS;
                offset -= SET_CMD_VALUE(rx_data[0]);
            }  
        }
        store_config();
    }else{
        // command info
        cmd_info->rx_data_len = 1;
        cmd_info->cmd = module_set_offset;
        cmd_info->cmd_callback = set_offset;
    }
}

void set_vtrim(uint8_t* rx_data,uint8_t* tx_data,cmd_info_t* cmd_info)
{
    if(cmd_info == NULL){
        if(SET_CMD_VALUE(rx_data[0]) >= 0 && SET_CMD_VALUE(rx_data[0]) < 64){
            if(SET_CMD_IS_ABS(rx_data[0])){
                vtrim = SET_CMD_VALUE(rx_data[0]);
            }else if(SET_CMD_IS_INC(rx_data[0])){
                vtrim += SET_CMD_VALUE(rx_data[0]);
                if(vtrim >= 64) vtrim -= 64;
            }else if(SET_CMD_IS_DEC(rx_data[0])){
                if(vtrim < SET_CMD_VALUE(rx_data[0])) vtrim += 64;
                vtrim -= SET_CMD_VALUE(rx_data[0]);
            }  
        }
        store_config();
    }else{
        // command info
        cmd_info->rx_data_len = 1;
        cmd_info->cmd = module_set_vtrim;
        cmd_info->cmd_callback = set_vtrim;
    }
}

// Delay the new pwm value with a certain delay. 
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

    if(distance < 0) distance += NUM_CHARS;
    pwm = 0;
    if(char_index != UNINITIALIZED && distance >= 0 && distance < NUM_CHARS){
        pwm = speed[distance];
    }


    CCPR1 = virtual_trim(pwm);
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
        for(uint16_t bit_i = 0x3FFF; data < bit_i ; bit_i = (bit_i << 1) & 0x3FFF){
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
        // INTCONbits.GIE = 0;
        // Unlock Sequence
        NVMCON2 = 0x55;
        NVMCON2 = 0xAA;
        NVMCON1bits.WR = 1;
    }

    NVMCON1bits.FREE = 0;
    NVMCON1bits.LWLO = 1;
    NVMCON1bits.WREN = 1;
    // INTCONbits.GIE = 0;
    for(int i = 0;i<31;i++){ 
        uint16_t  word = 0x3FFF;
        for(int j = 0; j < 14;j++){
            if(rec_remainder){
                word = (word << 1) & 0x3FFF; // shift 0 into word for each remainder
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
    // INTCONbits.GIE = 1;

    rev_cnt += rev_add;
    rev_add = 0;
}

void main(void)
{
    // init vars
    char_index = UNINITIALIZED;
    rev_add = 0;
    offset = 0;
    vtrim = 0;
    rev_cnt = 0;
    // init hw and peripherals
    init_hardware();
    // enable interrupts
    // INTCONbits.GIE = 1;
    // INTCONbits.PEIE = 1;
    // load default charset from NVM
    set_default_charset();
    // install callbacks for uart chain-comm commands
    install_command(do_nothing);
    install_command(goto_btl);
    install_command(get_config);
    install_command(get_fw_version);
    install_command(set_char);
    install_command(get_char);
    install_command(get_rev_cnt);
    install_command(set_charset);
    install_command(get_charset);
    install_command(set_offset);
    install_command(set_vtrim);
    // load revolution counter
    rev_cnt = calc_rev_cnt();
    uint32_t idle_timer = 0; // timeout counter
    int distance_to_rotate = 0;
    // main loop
    while(1){
        CLRWDT();
        chain_comm_loop(&idle_timer);
        if(idle_timer >= 10000 || distance_to_rotate) idle_timer = 0; // 1 seconds 
        if(idle_timer < 2500){  // 250ms -> Not idle
            distance_to_rotate = motor_control();
        }else{          // Idle
            PORTAbits.RA0 = 0; // disable IR led when idle
            // increment_rev_cnt();
        }
    }
}