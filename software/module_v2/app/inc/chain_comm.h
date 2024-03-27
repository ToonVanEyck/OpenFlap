#ifndef CHAIN_COMM_H
#define CHAIN_COMM_H

#include "chain_comm_abi.h"
#include "platform.h"

#ifdef IS_BTL
#define CMD_SIZE (module_goto_app + 1)
#else
#define CMD_SIZE (end_of_command + 1)
#endif

// typedef enum {
//     dataAvailable,
//     transmitComplete,
//     communicationTimeout,
// } chainCommEvent_t;

// typedef enum {
//     receiveHeader = 0,
//     indexModules,
//     receiveData,
//     passthrough,
//     transmitData,
//     receiveAcknowledge,
//     errorIgnoreData,
//     waitForAcknowledge,
// } chainCommState_t;

// typedef struct {
//     chainCommState_t state;
//     uint16_t index;
//     chainCommHeader_t header;
//     uint8_t cnt;
//     uint8_t data[CHAIN_COM_MAX_LEN];
// } chainCommCtx_t;

// extern const propertyHandler_t propertyHandlers[end_of_properties];

#define GENERATE_STATE_ENUM(ENUM, NAME) ENUM,
#define GENERATE_STATE_NAME(ENUM, NAME) NAME,

#define CHAIN_COMM_STATE(GENERATOR)                                                                                    \
    GENERATOR(rxHeader, "rxHeader")                                                                                    \
    GENERATOR(readAll_rxCnt, "readAll_rxCnt")                                                                          \
    GENERATOR(readAll_rxData, "readAll_rxData")                                                                        \
    GENERATOR(readAll_txData, "readAll_txData")                                                                        \
    GENERATOR(writeAll_rxData, "writeAll_rxData")                                                                      \
    GENERATOR(writeAll_exec, "writeAll_exec")                                                                          \
    GENERATOR(passthrough, "passthrough")

typedef enum { CHAIN_COMM_STATE(GENERATE_STATE_ENUM) } chainCommState_v2_t;
static const char *chainCommStateNames[] = {CHAIN_COMM_STATE(GENERATE_STATE_NAME)};

typedef void (*property_callback)(uint8_t *buf);

typedef struct {
    property_callback get;
    property_callback set;
} propertyHandler_t;

typedef enum {
    rx_event,
    tx_event,
    timeout_event,
} chainCommEvent_t;

typedef struct {
    chainCommState_v2_t state;
    chainCommHeader_t header;
    uint8_t rx_cnt;
    uint8_t tx_cnt;
    uint16_t index;
    uint8_t property_data[CHAIN_COM_MAX_LEN];
    propertyHandler_t property_handler[end_of_properties];
} chainCommCtx_v2_t;

// void chainCommRun(uint32_t *idle_timeout);
// void chainComm(chainCommEvent_t event);

bool chain_comm(chainCommCtx_v2_t *ctx, uint8_t *data, chainCommEvent_t event);

#endif
