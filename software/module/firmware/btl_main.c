#include "hardware_configuration.h"

#include <stdint.h>
#include <string.h>

#include "btl_properties.h"
#include "flash.h"

uint8_t app_valid = 0;

__interrupt() void isr(void)
{
    app__isr();
}

void writeFlashPage(uint8_t *data)
{
    clearAndWriteFlash(((uint16_t)data[0] << 8) | data[1], data + 2);
}

void do_command(uint8_t *data)
{
    switch (data[0]) {
        case (runApp_command):
            if (validateCheckSum()) {
                app_start();
            } else {
            }
            break;
        default:
            // no action
            break;
    }
}

void main(void)
{
    // INTCONbits.GIE = 0;
    init_hardware();

    TX_BYTE(0x00); // send ack to indicate successful boot
    TX_WAIT_DONE;

    uint32_t idle_timer = 0; // timeout counter
    while (1) {
        CLRWDT();
        chainCommRun(&idle_timer);
        idle_timer = 0; // not used in bootloader
    }
}
