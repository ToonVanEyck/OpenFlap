#include "chain_comm.h"

cmd_info_t cmd_info[CMD_SIZE] = {{0,0,NULL}};

uint8_t watch_tx = 0;

void chain_comm_loop(uint32_t *idle_timeout)
{
    static uint16_t comm_timer = 0; // timeout counter
    CLOCK_GUARD;
    (*idle_timeout)++;
    if(RX_DONE){
        *idle_timeout = 0;
        comm_timer = 0;
        chain_comm(comm_rx_data);
    }
    if(watch_tx && TX_DONE){
        *idle_timeout = 0;
        comm_timer = 0;
        watch_tx = 0;
        chain_comm(comm_tx_data);
    }
    if(comm_timer++ >= 500){ // 50ms
        comm_timer = 0;
        chain_comm(comm_timeout);
    } 
}

void chain_comm(uint8_t new_comm_data)
{
    static comm_ctx_t ctx = {.command = 0, .rx_data_cnt = 0, .tx_data_cnt = 0, .carry = 0, .rx_data = {0}, .tx_data = {0},.state = comm_state_command};
    static int init = 1;
    static int reset_cnt = 0;
    if(init){ // "install" read command
        cmd_info[module_read_data].rx_data_len = 3;
        cmd_info[module_read_data].cmd = module_read_data;
        cmd_info[module_read_data].cmd_callback = NULL;
        init = 0;
    }
    DEBUG_PRINT("---------------------------\n");
    DEBUG_PRINT("comm: %s\n", new_comm_data < comm_timeout? new_comm_data < comm_tx_data? "rx" : "tx" : "time");
    DEBUG_PRINT("state: %s\n", ctx.state < 3 ? ctx.state < 2? ctx.state < 1? "comm_state_command" : "comm_state_data" : "comm_state_passthrough" : "comm_state_transmit");
    switch (new_comm_data){
        case comm_rx_data:;
            reset_cnt = 0;
            uint8_t data;
            RX_BYTE(data);
            DEBUG_PRINT("<< rx data: 0x%02x\n", data);
            if(ctx.state == comm_state_command){
                ctx.command = data;
                if((ctx.command & CMD_CMD) == module_read_data) ctx.command |= CMD_EXTEND; // set the extend bit if it is a read command;
                DEBUG_PRINT("New Command: extend: %d command: %02d\n", (ctx.command & CMD_EXTEND)>1, (ctx.command & CMD_CMD));
                ctx.state = comm_state_data;
            }else if(ctx.state == comm_state_data){
                if((ctx.command & CMD_CMD) == module_read_data && ctx.rx_data_cnt < 2){
                    ctx.carry = ((ctx.carry && !++data) || (!ctx.rx_data_cnt && !++data));
                }
                ctx.rx_data[ctx.rx_data_cnt++] = data;
            }
            if(ctx.command & CMD_EXTEND || ctx.state == comm_state_passthrough){
                if(ctx.state == comm_state_transmit){
                    uint16_t node_cnt = ((uint16_t)ctx.rx_data[1]<<8) + (ctx.rx_data[0]);
                    DEBUG_PRINT("tx prev data: %d/%d  %d/%d\n", ctx.tx_data_cnt+1,ctx.rx_data[2],ctx.tx_node_cnt+1,node_cnt);
                    if(ctx.tx_node_cnt+1 < node_cnt){
                        TX_BYTE(data);
                        DEBUG_PRINT(">> tx data: 0x%02x\n", data);
                        if(++ctx.tx_data_cnt == ctx.rx_data[2]){
                            ctx.tx_data_cnt = 0;
                            ctx.tx_node_cnt++;
                        }
                    }
                }else{
                    TX_BYTE(data);
                    DEBUG_PRINT(">> tx data: 0x%02x\n", data);
                }
            }
            DEBUG_PRINT("rx data: %d/%d\n", ctx.rx_data_cnt,cmd_info[ctx.command & CMD_CMD].rx_data_len);
            if(ctx.state == comm_state_data && ctx.rx_data_cnt == cmd_info[ctx.command & CMD_CMD].rx_data_len){ // is all data received?
                if(ctx.command & CMD_EXTEND){
                    if(cmd_info[ctx.command & CMD_CMD].cmd_callback != NULL){
                        TX_WAIT_DONE; // transmit last command;
                        cmd_info[ctx.command & CMD_CMD].cmd_callback(ctx.rx_data,ctx.tx_data, NULL);
                    } 
                    if((ctx.command & CMD_CMD) == module_read_data){ // transmit
                        ctx.state = comm_state_transmit; 
                        ctx.rx_data_cnt = 0;
                    }else{ // reset
                        ctx.command = module_do_nothing;
                        ctx.state = comm_state_command;
                    }
                    ctx.rx_data_cnt = 0;
                    ctx.tx_data_cnt = 0;
                    ctx.tx_node_cnt = 0;
                    ctx.carry = 0;
                }else{
                    ctx.state = comm_state_passthrough;
                }
            }
            break;
        case comm_tx_data:;
            reset_cnt = 0;
            if(ctx.state == comm_state_transmit){
                uint16_t node_cnt = ((uint16_t)ctx.rx_data[1]<<8) + (ctx.rx_data[0]);
                if(ctx.tx_node_cnt+1 >= node_cnt){
                    DEBUG_PRINT("tx data: %d/%d\n", ctx.tx_data_cnt+1,ctx.rx_data[2]);
                    TX_BYTE(ctx.tx_data[ctx.tx_data_cnt++]);
                    TX_WAIT_DONE;
                    DEBUG_PRINT(">> tx data: 0x%02x\n", data);
                    if(ctx.tx_data_cnt == ctx.rx_data[2]){
                        ctx.command = module_do_nothing;
                        ctx.state = comm_state_command;
                        ctx.rx_data_cnt = 0;
                        ctx.tx_data_cnt = 0;
                        ctx.tx_node_cnt = 0;
                        ctx.carry = 0;
                        for(int i = 0;i<256;i++)ctx.tx_data[i]=0;
                    }
                }
            }
            break;
        case comm_timeout:;
                if(ctx.state == comm_state_passthrough && cmd_info[ctx.command & CMD_CMD].cmd_callback != NULL){
                    TX_WAIT_DONE; // transmit last command;
                    cmd_info[ctx.command & CMD_CMD].cmd_callback(ctx.rx_data,ctx.tx_data, NULL);
                } 
                ctx.command = module_do_nothing;
                ctx.state = comm_state_command;
                ctx.rx_data_cnt = 0;
                ctx.tx_data_cnt = 0;
                ctx.carry = 0;
            break;
    }
}

void install_command(void (*cmd_callback)(uint8_t*,uint8_t*,cmd_info_t*))
{
    cmd_info_t new_cmd_info;
    cmd_callback(NULL, NULL, &new_cmd_info);
    memcpy(&(cmd_info[new_cmd_info.cmd]),&new_cmd_info,sizeof(cmd_info_t));
}

