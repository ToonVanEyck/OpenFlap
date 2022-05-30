#ifndef FLAP_COMMAND_H
#define FLAP_COMMAND_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "chain_comm_abi.h"


#define CMD_COMM_BUF_LEN 2048
#define EXTEND (0x80)

typedef struct{
    size_t data_len;
    size_t data_offset;
    size_t total_data_len;
    char data[CMD_COMM_BUF_LEN];
}controller_queue_data_t;

#endif