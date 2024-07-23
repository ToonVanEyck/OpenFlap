#include "chain_comm.h"
#include "property_handlers.h"

#ifndef GENERATE_STATE_NAMES
#define GENERATE_STATE_NAMES false
#endif

/*
 * Set to true to enable UART RTT trace this will slow down the execution and might cause issues with the chain
 * communication.
 */
#define TRACE_CHAIN_COMM_UART false

#define CHAIN_COMM_TIMEOUT_MS 250

const char *get_state_name(uint8_t state);
void chain_comm_state_change(chain_comm_ctx_t *ctx, chain_comm_state_t state);
void chain_comm_exec(chain_comm_ctx_t *ctx);

/* Chain comm timer functions. */
void chain_comm_timer_start(chain_comm_ctx_t *ctx);
bool chain_comm_timer_elapsed(chain_comm_ctx_t *ctx);

/* Chain comm FSM state implementations. */
void chain_comm_state_rxHeader(chain_comm_ctx_t *ctx);
void chain_comm_state_readAll_rxCnt(chain_comm_ctx_t *ctx);
void chain_comm_state_readAll_rxData(chain_comm_ctx_t *ctx);
void chain_comm_state_readAll_txData(chain_comm_ctx_t *ctx);
void chain_comm_state_writeAll_rxData(chain_comm_ctx_t *ctx);
void chain_comm_state_writeAll_rxAck(chain_comm_ctx_t *ctx);
void chain_comm_state_writeSeq_rxData(chain_comm_ctx_t *ctx);
void chain_comm_state_writeSeq_rxToTx(chain_comm_ctx_t *ctx);

void chain_comm_init(chain_comm_ctx_t *ctx, uart_driver_ctx_t *uart)
{
    ctx->uart = uart;
    ctx->state = rxHeader;
    ctx->data_cnt = 0;
    ctx->index = 0;
    ctx->ack = false;
}

bool chain_comm(chain_comm_ctx_t *ctx)
{
    switch (ctx->state) {
        case rxHeader:
            chain_comm_state_rxHeader(ctx);
            break;
        case readAll_rxCnt:
            chain_comm_state_readAll_rxCnt(ctx);
            break;
        case readAll_rxData:
            chain_comm_state_readAll_rxData(ctx);
            break;
        case readAll_txData:
            chain_comm_state_readAll_txData(ctx);
            break;
        case writeAll_rxData:
            chain_comm_state_writeAll_rxData(ctx);
            break;
        case writeAll_rxAck:
            chain_comm_state_writeAll_rxAck(ctx);
            break;
        case writeSeq_rxData:
            chain_comm_state_writeSeq_rxData(ctx);
            break;
        case writeSeq_rxToTx:
            chain_comm_state_writeSeq_rxToTx(ctx);
            break;
        default:
            break;
    }
    return true;
}

/**
 * \brief Get the state name for a FSM state.
 *
 * \return the state name.
 */
const char *get_state_name(uint8_t state)
{
#if GENERATE_STATE_NAMES
    static const char *chain_comm_state_names[] = {CHAIN_COMM_STATE(GENERATE_STATE_NAME)};
    if (state < sizeof(chain_comm_state_names) / sizeof(chain_comm_state_names[0])) {
        return chain_comm_state_names[state];
    }
#endif
    return "undefined";
}

void chain_comm_timer_start(chain_comm_ctx_t *ctx)
{
    ctx->timeout_tick_cnt = HAL_GetTick() + CHAIN_COMM_TIMEOUT_MS;
}

bool chain_comm_timer_elapsed(chain_comm_ctx_t *ctx)
{
    return HAL_GetTick() > ctx->timeout_tick_cnt;
}

bool chain_comm_is_busy(chain_comm_ctx_t *ctx)
{
    return (ctx->state != rxHeader) || uart_driver_is_busy(ctx->uart);
}

/**
 * \brief Changes the state of the chain communication FSM.
 *
 * This function changes the state of the chain communication FSM to the provided state.
 * It also resets the rx and tx counters of the context.
 *
 * \param[inout] ctx Pointer to the chain_comm_ctx_t structure containing the context information.
 * \param[in] state The new state of the chain communication FSM.
 */
void chain_comm_state_change(chain_comm_ctx_t *ctx, chain_comm_state_t state)
{
#if GENERATE_STATE_NAMES
    debug_io_log_debug("State: %s -> %s\n", get_state_name(ctx->state), get_state_name(state));
#endif
    if (state != rxHeader) {
        chain_comm_timer_start(ctx);
    }
    ctx->state = state;
    ctx->data_cnt = 0;
}

/**
 * \brief Executes the chain communication based on the provided context.
 *
 * This function handles the execution of chain communication based on the provided context.
 * It checks the action field of the context header and performs the corresponding action.
 *
 * \param[in] ctx Pointer to the chain_comm_ctx_t structure containing the context information.
 */
void chain_comm_exec(chain_comm_ctx_t *ctx)
{
    const char *property_name = get_property_name(ctx->header.field.property);
    if (ctx->header.field.action == property_readAll) {
        if (ctx->property_handler[ctx->header.field.property].get) {
            ctx->property_handler[ctx->header.field.property].get(ctx->property_data);
            debug_io_log_debug("Read %s property\n", property_name);
        } else {
            debug_io_log_debug("Read %s property not supported\n", property_name);
        }
    } else if (ctx->header.field.action == property_writeAll || ctx->header.field.action == property_writeSequential) {
        if (ctx->property_handler[ctx->header.field.property].set) {
            ctx->property_handler[ctx->header.field.property].set(ctx->property_data);
            debug_io_log_debug("Write %s property\n", property_name);
        } else {
            debug_io_log_debug("Write %s property not supported\n", property_name);
        }
    }
}

/**
 * \brief Handles the rxHeader state of the chain communication FSM.
 *
 * This state receives a header byte and changes the state based on the action field of the header.
 *
 * \param[inout] ctx Pointer to the chain_comm_ctx_t structure containing the context information.
 * \param[in] data Pointer to the data received.
 * \param[in] event The event type.
 *
 * \return True if data needs to be transmitted, false otherwise.
 */
void chain_comm_state_rxHeader(chain_comm_ctx_t *ctx)
{
    uint8_t data;
    if (uart_driver_cnt_writable(ctx->uart) && uart_driver_read(ctx->uart, &data, 1)) {
        ctx->header.raw = data;
        switch (ctx->header.field.action) {
            case property_readAll:
                chain_comm_state_change(ctx, readAll_rxCnt);
                uart_driver_write(ctx->uart, &data, 1);
                break;
            case property_writeAll:
                chain_comm_state_change(ctx, writeAll_rxData);
                uart_driver_write(ctx->uart, &data, 1);
                break;
            case property_writeSequential:
                chain_comm_state_change(ctx, writeSeq_rxData);
                break;
            case do_nothing:
                chain_comm_state_change(ctx, rxHeader);
                break;
        }
    }
}

/**
 * \brief Handles the readAll_rxCnt state of the chain communication FSM.
 *
 * This state receives 2 count bytes which the modules increment, this allows each successive module to know how many
 * modules are in the chain before this one. As soon as the second byte is received, the module will execute property
 * read callback.
 *
 * \param[inout] ctx Pointer to the chain_comm_ctx_t structure containing the context information.
 * \param[in] data Pointer to the data received.
 * \param[in] event The event type.
 *
 * \return True if data needs to be transmitted, false otherwise.
 */
void chain_comm_state_readAll_rxCnt(chain_comm_ctx_t *ctx)
{
    uint8_t data;
    if (uart_driver_cnt_writable(ctx->uart) && uart_driver_read(ctx->uart, &data, 1)) {
        ctx->data_cnt++;
        if (ctx->data_cnt == 1) {
            ctx->index += ((uint16_t)data) + 1;
            data = (ctx->index >> 0) & 0xff;
        } else {
            ctx->index += (data << 8);
            data = (ctx->index >> 8) & 0xff;
            chain_comm_exec(ctx);
            if (--ctx->index) {
                chain_comm_state_change(ctx, readAll_rxData);
            } else {
                chain_comm_state_change(ctx, readAll_txData);
            }
        }
        uart_driver_write(ctx->uart, &data, 1);
    } else if (chain_comm_timer_elapsed(ctx)) {
        chain_comm_state_change(ctx, rxHeader);
    }
}

/**
 * \brief Handles the readAll_rxData state of the chain communication FSM.
 *
 * This state receives the data bytes written by the readAll operation of the previous module. These bytes are forwarded
 * to the next modules. Once all data of the previous modules has been forwarded, the module will change the state to
 * readAll_txData.
 *
 * \param[inout] ctx Pointer to the chain_comm_ctx_t structure containing the context information.
 * \param[in] data Pointer to the data received.
 * \param[in] event The event type.
 *
 * \return True if data needs to be transmitted, false otherwise.
 */
void chain_comm_state_readAll_rxData(chain_comm_ctx_t *ctx)
{
    uint8_t data;
    while (uart_driver_cnt_writable(ctx->uart) && ctx->data_cnt < get_property_size(ctx->header.field.property) &&
           uart_driver_read(ctx->uart, &data, 1)) {
        ctx->data_cnt++;
        uart_driver_write(ctx->uart, &data, 1);
    }
    if (ctx->data_cnt == get_property_size(ctx->header.field.property)) {
        if (--ctx->index) {
            chain_comm_state_change(ctx, readAll_rxData);
        } else {
            chain_comm_state_change(ctx, readAll_txData);
        }
    } else if (chain_comm_timer_elapsed(ctx)) {
        chain_comm_state_change(ctx, rxHeader);
    }
}

/**
 * \brief Handles the readAll_txData state of the chain communication FSM.
 *
 * This state sends the data bytes of the readAll operation to the next module. Once all data has been sent, the state
 * will change to rxHeader.
 *
 * \param[inout] ctx Pointer to the chain_comm_ctx_t structure containing the context information.
 * \param[in] data Pointer to the data received.
 * \param[in] event The event type.
 *
 * \return True if data needs to be transmitted, false otherwise.
 */
void chain_comm_state_readAll_txData(chain_comm_ctx_t *ctx)
{
    uint8_t data_cnt = uart_driver_write(ctx->uart, &ctx->property_data[ctx->data_cnt],
                                         get_property_size(ctx->header.field.property) - ctx->data_cnt);
    ctx->data_cnt += data_cnt;
    if (ctx->data_cnt == get_property_size(ctx->header.field.property)) {
        chain_comm_state_change(ctx, rxHeader);
    } else if (chain_comm_timer_elapsed(ctx)) {
        chain_comm_state_change(ctx, rxHeader);
    }
}

/**
 * \brief Handles the writeAll_rxData state of the chain communication FSM.
 *
 * This state receives the data bytes to be written by the writeAll operation. All data is received is also passed to
 * the next module. Once all data has been forwarded to the next module, the module will execute the writeAll command
 * for the property and change the state to writeAll_rxAck.
 *
 * \param[inout] ctx Pointer to the chain_comm_ctx_t structure containing the context information.
 * \param[in] data Pointer to the data received.
 * \param[in] event The event type.
 *
 * \return True if data needs to be transmitted, false otherwise.
 */
void chain_comm_state_writeAll_rxData(chain_comm_ctx_t *ctx)
{
    while (uart_driver_cnt_writable(ctx->uart) && ctx->data_cnt < get_property_size(ctx->header.field.property) &&
           uart_driver_read(ctx->uart, &ctx->property_data[ctx->data_cnt], 1)) {
        if (!uart_driver_write(ctx->uart, &ctx->property_data[ctx->data_cnt], 1)) {
            debug_io_log_error("Error writing data to uart\n");
        };
        ctx->data_cnt++;
    }
    if (ctx->data_cnt == get_property_size(ctx->header.field.property) &&
        uart_driver_cnt_written(ctx->uart) == 0) { // All bytes have been transmitted.
        chain_comm_exec(ctx);
        chain_comm_state_change(ctx, writeAll_rxAck);
    } else if (chain_comm_timer_elapsed(ctx)) {
        chain_comm_state_change(ctx, rxHeader);
    }
}
/**
 * \brief Handles the writeAll_rxAck state of the chain communication FSM.
 *
 * This state waits for an acknowledgement from the previous module. Once the acknowledgement has been received, the
 * state will change to rxHeader.
 *
 * \param[inout] ctx Pointer to the chain_comm_ctx_t structure containing the context information.
 * \param[in] data Pointer to the data received.
 * \param[in] event The event type.
 *
 * \return True if data needs to be transmitted, false otherwise.
 */
void chain_comm_state_writeAll_rxAck(chain_comm_ctx_t *ctx)
{
    uint8_t data;
    if (uart_driver_cnt_writable(ctx->uart) && uart_driver_read(ctx->uart, &data, 1)) {
        if (data == 0x00) {
            uart_driver_write(ctx->uart, &data, 1);
            chain_comm_state_change(ctx, rxHeader);
        }
    } else if (chain_comm_timer_elapsed(ctx)) {
        chain_comm_state_change(ctx, rxHeader);
    }
}

/**
 * \brief Handles the writeSeq_rxData state of the chain communication FSM.
 *
 * This state receives the data bytes to be written by the writeSequential operation. The received bytes are NOT passed
 * to sequential modules. Once the data required for the writeSequential operation has been received, the state will
 * change to writeSeq_rxToTx.
 *
 * \param[inout] ctx Pointer to the chain_comm_ctx_t structure containing the context information.
 * \param[in] data Pointer to the data received.
 * \param[in] event The event type.
 *
 * \return True if data needs to be transmitted, false otherwise.
 */
void chain_comm_state_writeSeq_rxData(chain_comm_ctx_t *ctx)
{
    if (uart_driver_cnt_writable(ctx->uart) && uart_driver_read(ctx->uart, &ctx->property_data[ctx->data_cnt], 1)) {
        uart_driver_write(ctx->uart, &ctx->property_data[ctx->data_cnt], 1);
        if (++ctx->data_cnt == get_property_size(ctx->header.field.property)) {
            chain_comm_state_change(ctx, writeSeq_rxToTx);
        }
    } else if (chain_comm_timer_elapsed(ctx)) {
        chain_comm_state_change(ctx, rxHeader);
    }
}

/**
 * \brief Handles the writeSeq_rxToTx state of the chain communication FSM.
 *
 * This state will forward all received data to the next module. This will happen until the timeout event occurs. Once
 * the timeout has occurred, the module will execute the writeSequential command for the property and change the state
 * to rxHeader.
 *
 * \param[inout] ctx Pointer to the chain_comm_ctx_t structure containing the context information.
 * \param[in] data Pointer to the data received.
 * \param[in] event The event type.
 *
 * \return True if data needs to be transmitted, false otherwise.
 */
void chain_comm_state_writeSeq_rxToTx(chain_comm_ctx_t *ctx)
{
    uint8_t data;
    if (uart_driver_cnt_writable(ctx->uart) && uart_driver_read(ctx->uart, &data, 1)) {
        uart_driver_write(ctx->uart, &data, 1);
        chain_comm_timer_start(ctx);
    } else if (chain_comm_timer_elapsed(ctx)) {
        chain_comm_exec(ctx);
        chain_comm_state_change(ctx, rxHeader);
    }
}