#pragma once

#include "chain_comm_abi.h"
#include "platform.h"

#ifdef IS_BTL
#define CMD_SIZE (module_goto_app + 1)
#else
#define CMD_SIZE (end_of_command + 1)
#endif

#define GENERATE_STATE_ENUM(ENUM, NAME) ENUM,
#define GENERATE_STATE_NAME(ENUM, NAME) NAME,

#define CHAIN_COMM_STATE(GENERATOR)                                                                                    \
    GENERATOR(rxHeader, "rxHeader")                                                                                    \
    GENERATOR(readAll_rxCnt, "readAll_rxCnt")                                                                          \
    GENERATOR(readAll_rxData, "readAll_rxData")                                                                        \
    GENERATOR(readAll_txData, "readAll_txData")                                                                        \
    GENERATOR(writeAll_rxData, "writeAll_rxData")                                                                      \
    GENERATOR(writeSeq_rxData, "writeSeq_rxData")                                                                      \
    GENERATOR(writeSeq_rxToTx, "writeSeq_rxToTx")

typedef enum { CHAIN_COMM_STATE(GENERATE_STATE_ENUM) } chain_comm_state_t;

#ifdef DO_GENERATE_STATE_NAMES
static const char *chain_comm_state_names[] = {CHAIN_COMM_STATE(GENERATE_STATE_NAME)};
#endif

typedef void (*property_callback)(uint8_t *buf);

typedef struct {
    property_callback get;
    property_callback set;
} property_handler_t;

typedef enum {
    rx_event,
    tx_event,
    timeout_event,
} chain_comm_event_t;

typedef struct {
    chain_comm_state_t state;
    chainCommHeader_t header;
    uint8_t rx_cnt;
    uint8_t tx_cnt;
    uint16_t index;
    uint8_t property_data[CHAIN_COM_MAX_LEN];
    property_handler_t property_handler[end_of_properties];
} chain_comm_ctx_t;

/**
 * \brief Executes the chain communication based on the provided context.
 *
 * This function handles the execution of chain communication based on the provided context.
 * It checks the state of the context and calls the corresponding state handler. Additionally, it increments the rx or
 * tx counter.
 *
 * \param[inout] ctx Pointer to the #chain_comm_ctx_t structure containing the context information.
 * \param[in] data Pointer to the data received.
 * \param[in] event The event type.
 *
 * \return True if data needs to be transmitted, false otherwise.
 */
bool chain_comm(chain_comm_ctx_t *ctx, uint8_t *data, chain_comm_event_t event);