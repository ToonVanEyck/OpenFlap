#pragma once

#include "chain_comm_abi.h"
#include "display.h"

typedef struct {
    display_t *display; /**< Display context. */
    // chain_comm_msg_t msg;
} chain_comm_ctx_t;