#include "hardware_configuration.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <xc.h>

#include "app_properties.h"
#include "flash.h"
#include "stattic_assert.h"
#include "version.h"

#define COL_END PORTAbits.RA5
#define MOTOR_DISABLE TRISAbits.TRISA2

typedef struct __attribute__((__packed__)) {
    char characterMap[4 * NUM_CHARS];
    uint8_t offset;
    uint8_t vtrim;
    uint8_t baseSpeed;
    uint8_t reserved[29];
} nonVolatileConfig_t;

// nonVolatileConfig_t struct size must be equal to the reserved config size.
STATIC_ASSERT(sizeof(nonVolatileConfig_t) == CONFIG_SIZE)

typedef struct __attribute__((__packed__)) {
    nonVolatileConfig_t nvm;
    struct {
        uint8_t charIndex;
        uint8_t speedMultiplier;
    } vm;
} moduleConfig_t;

static moduleConfig_t config;

// Read (14bit) words from nvm and convert it to 28 (16bit) words.
// buf_size must be a multiple of 7
void readConfig(nonVolatileConfig_t *config)
{
    uint16_t *data = (uint16_t *)config;
    uint16_t nvmData[8] = {0};
    uint8_t o = 0;
    for (uint16_t i = 0; i < CONFIG_PAGES * 32; i++) {
        uint16_t j = i - (i >> 3);
        nvmData[i % 8] = readFlash(CONFIG_START_ADDR + i);
        if (i % 8 == 7) {
            // 8th 14-bit word contains the 2 msb's for 7 first 14bit words
            data[--j] = nvmData[6] | ((nvmData[7] << 2) & 0xC000);
            data[--j] = nvmData[5] | ((nvmData[7] << 4) & 0xC000);
            data[--j] = nvmData[4] | ((nvmData[7] << 6) & 0xC000);
            data[--j] = nvmData[3] | ((nvmData[7] << 8) & 0xC000);
            data[--j] = nvmData[2] | ((nvmData[7] << 10) & 0xC000);
            data[--j] = nvmData[1] | ((nvmData[7] << 12) & 0xC000);
            data[--j] = nvmData[0] | ((nvmData[7] << 14) & 0xC000);
        }
        // data[i] = readFlash(CONFIG_START_ADDR + i);
    }
}

// write 28 (16bit words) in to 1 page of 32 (14bit) words.
void writeConfig(nonVolatileConfig_t *config)
{
    uint16_t *data = (uint16_t *)config;
    uint16_t nvmData[32] = {0x3FFF};
    for (uint16_t i = 0; i < CONFIG_PAGES * 32; i++) {
        uint16_t j = i - (i >> 3);
        nvmData[i % 32] = (data[j] /* & 0x3FFF*/);
        if (i % 8 == 7) {
            nvmData[i % 32] = ((data[--j] & 0xC000) >> 14) | ((data[--j] & 0xC000) >> 12) |
                              ((data[--j] & 0xC000) >> 10) | ((data[--j] & 0xC000) >> 8) | ((data[--j] & 0xC000) >> 6) |
                              ((data[--j] & 0xC000) >> 4) | ((data[--j] & 0xC000) >> 2);
        }
        nvmData[i % 32] &= 0x3FFF;
        if (i % 32 == 31) {
            clearAndWriteFlash((CONFIG_START_ADDR + i - 31) << 1, (uint8_t *)nvmData);
        }
    }
}

#define IR_OFF_TICKS 97 // 770
#define IR_ON_TICKS 3   // 20

uint8_t read_encoder(uint8_t is_idle)
{
    static unsigned pulse_cnt = IR_OFF_TICKS;
    static uint8_t enc_res = 0xff;
    static uint8_t enc_buffer[3] = {0};
    if (++pulse_cnt >= IR_OFF_TICKS) {
        // PORTAbits.RA0 = 1; // Enable IR LEDs
        CCPR2 = 1000;
        if (pulse_cnt >= IR_OFF_TICKS + IR_ON_TICKS) {
            pulse_cnt = 0;
            uint8_t enc_g = 0x3F;
            uint8_t enc_d = 0;
            enc_g ^= PORTCbits.RC3 << 5;
            enc_g ^= PORTCbits.RC2 << 4;
            enc_g ^= PORTCbits.RC4 << 3;
            enc_g ^= PORTCbits.RC1 << 2;
            enc_g ^= PORTCbits.RC5 << 1;
            enc_g ^= PORTCbits.RC0 << 0;
            // PORTAbits.RA0 = 0; // Disable IR LEDs
            CCPR2 = 0;
            // convert gray code to decimal
            for (enc_d = 0; enc_g; enc_g = enc_g >> 1)
                enc_d ^= enc_g;
            if (enc_d > (NUM_CHARS - 1))
                enc_d = (NUM_CHARS - 1);
            enc_d = (uint8_t)NUM_CHARS - enc_d - 1;
            // apply the encoder offset.
            enc_d += config.nvm.offset;
            if (enc_d >= NUM_CHARS)
                enc_d -= NUM_CHARS;
            // debounce readings
            enc_buffer[0] = is_idle ? enc_buffer[1] : (uint8_t)enc_d;
            enc_buffer[1] = is_idle ? enc_buffer[2] : (uint8_t)enc_d;
            enc_buffer[2] = (uint8_t)enc_d;
            if (enc_buffer[0] == enc_buffer[1] && enc_buffer[1] == enc_buffer[2])
                enc_res = enc_d;
        }
    }
    return (uint8_t)enc_res;
}

asm("GLOBAL _app__isr");     // ensure ISR is not optimized out by compiler
__at(0x0808) void app__isr() // "real" isr routine is located in bootloader, that isr will call this function at 0x0808
{
}

void do_command(uint8_t *data)
{
    switch (data[0]) {
        case (reboot_command):
            RESET();
            break;
        default:
            // no action
            break;
    }
}

void get_columnEnd(uint8_t *data)
{
    data[0] = COL_END;
}

void get_version(uint8_t *data)
{
    data[0] = git_version_major;
    data[1] = git_version_minor;
    data[2] = git_version_patch;
    data[3] = git_version_tweak;
    memcpy((char *)data + 4, git_version_hash, 7);
    data[11] = git_version_is_dirty;
}

void set_character(uint8_t *data)
{
    config.vm.charIndex = data[0];
}

void get_character(uint8_t *data)
{
    data[0] = read_encoder(1);
}

void get_characterMapSize(uint8_t *data)
{
    data[0] = NUM_CHARS;
}

void set_characterMap(uint8_t *data)
{
    memcpy(config.nvm.characterMap, (char *)data, 4 * NUM_CHARS);
    writeConfig(&config.nvm);
}

void get_characterMap(uint8_t *data)
{
    memcpy((char *)data, config.nvm.characterMap, 4 * NUM_CHARS);
}

void set_offset(uint8_t *data)
{
    if (data[0] >= 0 && data[0] < NUM_CHARS) {
        if (config.nvm.offset != data[0]) {
            config.nvm.offset = data[0];
            writeConfig(&config.nvm);
        }
    }
}

void get_offset(uint8_t *data)
{
    data[0] = config.nvm.offset;
}

void set_vtrim(uint8_t *data)
{

    if (config.nvm.vtrim != data[0]) {
        config.nvm.vtrim = data[0];
        writeConfig(&config.nvm);
    }
}

void get_vtrim(uint8_t *data)
{
    data[0] = config.nvm.vtrim;
}

void set_baseSpeed(uint8_t *data)
{
    if (config.nvm.baseSpeed != data[0]) {
        config.nvm.baseSpeed = data[0];
        writeConfig(&config.nvm);
    }
}

void get_baseSpeed(uint8_t *data)
{
    data[0] = config.nvm.baseSpeed;
}

uint16_t virtual_trim(uint16_t pwm_i)
{
    static uint16_t pwm_o = 0xFFFF;
    static uint16_t vtrim_count;
    if (pwm_o == 0xffff)
        pwm_o = pwm_i;
    if (pwm_i != pwm_o) {
        vtrim_count++;
        if (vtrim_count > ((uint16_t)config.nvm.vtrim * 20)) {
            vtrim_count = 0;
            pwm_o = pwm_i;
        }
    }
    return pwm_o;
}

int motor_control(void)
{
    static uint16_t pwm = 0;

    int distance = (int)config.vm.charIndex - read_encoder(pwm == 0);

    if (config.vm.charIndex == UNINITIALIZED)
        distance = 0;
    if (distance < 0)
        distance += NUM_CHARS;

    pwm = 0;
    if (distance > 0 && distance < NUM_CHARS) {
        pwm = config.nvm.baseSpeed + distance * config.vm.speedMultiplier;
    }
    CCPR1 = virtual_trim(pwm);
    return distance;
}

void main(void)
{
    // init vars
    config.vm.charIndex = UNINITIALIZED;
    config.vm.speedMultiplier = 7;
    // init hw and peripherals
    init_hardware();
    // enable interrupts
    // INTCONbits.GIE = 1;
    // INTCONbits.PEIE = 1;
    // load default characterMap from NVM
    readConfig(&config.nvm);

    uint32_t idle_timer = 0; // timeout counter
    int distance_to_rotate = 0;

    TX_BYTE(0x00); // send ack to indicate successful boot
    TX_WAIT_DONE;
    // main loop
    while (1) {
        CLRWDT();
        chainCommRun(&idle_timer);
        if (idle_timer >= 2500 || distance_to_rotate)
            idle_timer = 0;      // 1 seconds
        if (idle_timer < 2500) { // 250ms -> Not idle
            distance_to_rotate = motor_control();
        } else { // Idle
            // PORTAbits.RA0 = 0; // disable IR led when idle
            CCPR2 = 0;
        }
    }
}