#pragma once

#include "chain_comm_abi.h"
#include "platform.h"
#include "uart_driver.h"

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
    GENERATOR(writeSeq_rxToTx, "writeSeq_rxToTx")                                                                      \
    GENERATOR(writeAll_rxAck, "writeAll_rxAck")

typedef enum { CHAIN_COMM_STATE(GENERATE_STATE_ENUM) } chain_comm_state_t;

typedef void (*property_callback)(uint8_t *buf);

typedef struct {
    property_callback get;
    property_callback set;
} property_handler_t;

/**
 * \brief Chain-comm context object.
 */
typedef struct {
    uart_driver_ctx_t *uart;                  /**< Uart driver to be used by the protocol. */
    chain_comm_state_t state;                 /**< The current state of the FSM managing the protocol. */
    chainCommHeader_t header;                 /**< The header of the current message. */
    uint8_t data_cnt;                         /**< The number of bytes handled in the current state. */
    uint16_t index;                           /**< The index counter of the module in the display. */
    uint8_t property_data[CHAIN_COM_MAX_LEN]; /**< The data of the current property to be written or read. */
    property_handler_t property_handler[end_of_properties]; /**< The property handlers. */
    bool ack;                  /**< Flag indicating if the current message is waiting for an acknowledgment.  */
    uint32_t timeout_tick_cnt; /**< Counter for determining timeout. */
} chain_comm_ctx_t;

/**
 * \brief Initializes the chain communication context.
 *
 * This function initializes the chain communication context based on the provided UART driver.
 *
 * \param[inout] ctx Pointer to the #chain_comm_ctx_t structure containing the context information.
 * \param[in] uart Pointer to the UART driver.
 */
void chain_comm_init(chain_comm_ctx_t *ctx, uart_driver_ctx_t *uart);

/**
 * \brief Executes the chain communication based on the provided context.
 *
 * This function handles the execution of chain communication based on the provided context.
 * It checks the state of the context and calls the corresponding state handler.
 *
 * \param[inout] ctx Pointer to the #chain_comm_ctx_t structure containing the context information.
 *
 * \return True if data needs to be transmitted, false otherwise.
 */
bool chain_comm(chain_comm_ctx_t *ctx);

/**
 * \brief Checks if the chain communication is busy.
 * The chain communication is considered busy if the state is not #rxHeader or the uart driver is busy.
 *
 * \param[inout] ctx Pointer to the #chain_comm_ctx_t structure containing the context information.
 *
 * \return True if the chain communication is busy, otherwise false.
 */
bool chain_comm_is_busy(chain_comm_ctx_t *ctx);