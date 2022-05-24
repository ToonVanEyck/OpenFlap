#ifndef HARDWARE_CONFIGURATION_H
#define HARDWARE_CONFIGURATION_H

#include "config_bytes.h"
#include <xc.h>

#define _XTAL_FREQ 32000000

#define APP_START_ADDR      (0x800)
#define APP_END_ADDR        (0x1FFF - 96) // reserve 3 32 word pages for nvm data

#define INT_ADDR        APP_START_ADDR + 0x08
#define CS_ADDR         APP_END_ADDR - 1

#define REV_CNT_BASE_ADDR (0x1FE0)
#define REV_CNT_MULT_ADDR (REV_CNT_BASE_ADDR + 31)

#define CHARSET_BASE_ADDR (0x1FA0)

#define UNINITIALIZED 0xff
#define NUM_CHARS 48

extern void app_start() __at(APP_START_ADDR);
extern void app__isr()  __at(INT_ADDR);

void init_hardware(void);

#endif //HARDWARE_CONFIGURATION_H