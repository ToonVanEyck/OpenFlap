#pragma once

#include "chain_comm.h"
#include "config.h"
#include "platform.h"

/** Struct with helper variables. */
typedef struct openflap_ctx_tag {
    uint8_t flap_position;      /**< The current position of flap wheel. */
    uint8_t flap_setpoint;      /**< The desired position of flap wheel. */
    openflap_config_t config;   /**< The configuration data. */
    chain_comm_ctx_t chain_ctx; /**< The chain communication context. */
} openflap_ctx_t;
