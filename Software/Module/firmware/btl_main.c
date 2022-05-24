#include "hardware_configuration.h"

#include <stdint.h>
#include <string.h>

#include "chain_comm.h"

uint8_t app_valid = 0;

__interrupt() void isr(void)
{
    app__isr();
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
    // INTCONbits.GIE = 0;
    init_hardware();
    install_command(write_page);
    install_command(goto_app);
    uint32_t idle_timer = 0; // timeout counter
    while(1){
        CLRWDT();
        chain_comm_loop(&idle_timer); 
        idle_timer = 0; // not used in bootloader
    }
}
