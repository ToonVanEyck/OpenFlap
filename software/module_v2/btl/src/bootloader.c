
#include "debug_io.h"
#include "memory_map.h"
#include "py32f0xx_hal.h"
#include <stdint.h>

static __attribute__((naked)) void start_app(uint32_t pc, uint32_t sp)
{
    __asm("           \n\
          msr msp, r1 /* load r1 into MSP */\n\
          bx r0       /* branch to the address at r0 */\n\
    ");
}

int main(void)
{
    debug_io_init();
    debug_io_log_info("OpenFlap Bootloader!\n");
    uint32_t *app_code = (uint32_t *)__FLASH_APP_START__;
    uint32_t app_sp = app_code[0];
    uint32_t app_start = app_code[1];
    start_app(app_start, app_sp);
    // SCB->VTOR = SRAM_BASE;
    /* Not Reached */
    while (1) {
    }
}