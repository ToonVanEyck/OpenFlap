#include "chain_comm.h"
#include "property_handlers.h"

#ifndef CHAIN_COMM_DEBUG
#define CHAIN_COMM_DEBUG false
#endif

/*
 * Set to true to enable UART RTT trace this will slow down the execution and might cause issues with the chain
 * communication.
 */
#define TRACE_CHAIN_COMM_UART false

#if CHAIN_COMM_DEBUG
const char *get_state_name(uint8_t state);
#endif

void chain_comm_state_change(chain_comm_ctx_t *ctx, chain_comm_state_t state);
void chain_comm_exec(chain_comm_ctx_t *ctx);

/* Chain comm timer functions. */
void chain_comm_timer_start(chain_comm_ctx_t *ctx);
bool chain_comm_timer_elapsed(chain_comm_ctx_t *ctx);

/* Chain comm FSM state implementations. */
void chain_comm_state_rxHeader(chain_comm_ctx_t *ctx);
void chain_comm_state_rxSize(chain_comm_ctx_t *ctx);
void chain_comm_state_readAll_rxCnt(chain_comm_ctx_t *ctx);
void chain_comm_state_readAll_rxData(chain_comm_ctx_t *ctx);
void chain_comm_state_readAll_txSize(chain_comm_ctx_t *ctx);
void chain_comm_state_readAll_txData(chain_comm_ctx_t *ctx);
void chain_comm_state_writeAll_rxData(chain_comm_ctx_t *ctx);
void chain_comm_state_writeAll_rxAck(chain_comm_ctx_t *ctx);
void chain_comm_state_writeSeq_rxData(chain_comm_ctx_t *ctx);
void chain_comm_state_writeSeq_rxToTx(chain_comm_ctx_t *ctx);

void chain_comm_init(chain_comm_ctx_t *ctx, uart_driver_ctx_t *uart)
{
    ctx->uart     = uart;
    ctx->state    = rxHeader;
    ctx->data_cnt = 0;
    ctx->index    = 0;
    ctx->ack      = false;
}

bool chain_comm(chain_comm_ctx_t *ctx)
{
    switch (ctx->state) {
        case rxHeader:
            chain_comm_state_rxHeader(ctx);
            break;
        case rxSize:
            chain_comm_state_rxSize(ctx);
            break;
        case readAll_rxCnt:
            chain_comm_state_readAll_rxCnt(ctx);
            break;
        case readAll_rxData:
            chain_comm_state_readAll_rxData(ctx);
            break;
        case readAll_txSize:
            chain_comm_state_readAll_txSize(ctx);
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
            debug_io_log_error("Invalid state\n");
            break;
    }
    return true;
}

/**
 * \brief Get the state name for a FSM state.
 *
 * \return the state name.
 */
#if CHAIN_COMM_DEBUG
const char *get_state_name(uint8_t state)
{
    static const char *chain_comm_state_names[] = {CHAIN_COMM_STATE(GENERATE_STATE_NAME)};
    if (state < sizeof(chain_comm_state_names) / sizeof(chain_comm_state_names[0])) {
        return chain_comm_state_names[state];
    }
    return "undefined";
}
#endif

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
#if CHAIN_COMM_DEBUG
    debug_io_log_debug("State: %s -> %s\n", get_state_name(ctx->state), get_state_name(state));
#endif
    if (state != rxHeader) {
        chain_comm_timer_start(ctx);
    }
    ctx->state    = state;
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
    const char *property_name = chain_comm_property_name_by_id(ctx->header.property);
    if (ctx->header.action == property_readAll) {
        if (ctx->property_handler[ctx->header.property].get) {
            ctx->property_size = chain_comm_property_read_attributes_get(ctx->header.property)->static_property_size;
            ctx->property_handler[ctx->header.property].get(ctx->property_data, &ctx->property_size);
            debug_io_log_debug("Read %s property\n", property_name);
        } else {
            debug_io_log_debug("Read %s property not supported\n", property_name);
        }
    } else if (ctx->header.action == property_writeAll || ctx->header.action == property_writeSequential) {
        if (ctx->property_handler[ctx->header.property].set) {
            ctx->property_handler[ctx->header.property].set(ctx->property_data, &ctx->property_size);
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
 */
void chain_comm_state_rxHeader(chain_comm_ctx_t *ctx)
{
    uint8_t data;
    ctx->property_size = 0;
    if (uart_driver_cnt_writable(ctx->uart) && uart_driver_read(ctx->uart, &data, 1)) {
        ctx->header.raw = data;
        if (ctx->header.property >= PROPERTIES_MAX) {
            ctx->header.action = do_nothing;
        }

        /* Switch to next state. */
        switch (ctx->header.action) {
            case property_readAll:
                uart_driver_write(ctx->uart, &data, 1);
                chain_comm_state_change(ctx, readAll_rxCnt);
                break;
            case property_writeAll:
                uart_driver_write(ctx->uart, &data, 1);
                chain_comm_state_change(ctx, rxSize);
                break;
            case property_writeSequential:
                /* Skip to transparent mode when no property is defined. */
                chain_comm_state_change(ctx, ctx->header.property ? rxSize : writeSeq_rxToTx);
                break;
            default:
                chain_comm_state_change(ctx, rxHeader);
                break;
        }
    }
}

/**
 * \brief Handles the rxSize state of the chain communication FSM.
 *
 * This state set initiates the property size in the context, it either sets the property size based on known static
 * property sizes, or it receives the property size in case of a dynamic property size.
 *
 * \param[inout] ctx Pointer to the chain_comm_ctx_t structure containing the context information.
 */
void chain_comm_state_rxSize(chain_comm_ctx_t *ctx)
{
    const chain_comm_binary_attributes_t *prop_attr =
        (ctx->header.action == property_readAll) ? chain_comm_property_read_attributes_get(ctx->header.property)
                                                 : chain_comm_property_write_attributes_get(ctx->header.property);

    /* Get the dynamic size bytes and retransmit the for readAll and writeAll properties. */
    if (prop_attr->dynamic_property_size) {
        while (uart_driver_cnt_writable(ctx->uart) && ctx->data_cnt < 2 &&
               uart_driver_read(ctx->uart, &((uint8_t *)&ctx->property_size)[ctx->data_cnt], 1)) {
            if (ctx->header.action == property_writeAll || ctx->header.action == property_readAll) {
                uart_driver_write(ctx->uart, &((uint8_t *)&ctx->property_size)[ctx->data_cnt], 1);
            }
            ctx->data_cnt++;
        };
    } else {
        ctx->property_size = prop_attr->static_property_size;
    }

    /* Switch mode. */
    if (ctx->data_cnt == 2 || !prop_attr->dynamic_property_size) {
        debug_io_log_debug("Property size: %d\n", ctx->property_size);
        if (ctx->header.action == property_writeAll) {
            chain_comm_state_change(ctx, writeAll_rxData);
        } else if (ctx->header.action == property_writeSequential) {
            chain_comm_state_change(ctx, writeSeq_rxData);
        } else if (ctx->header.action == property_readAll) {
            chain_comm_state_change(ctx, readAll_rxData);
        }
    } else if (chain_comm_timer_elapsed(ctx)) {
        chain_comm_state_change(ctx, rxHeader);
    }
}

/**
 * \brief Handles the readAll_rxCnt state of the chain communication FSM.
 *
 * This state receives 2 count bytes which the modules increment, this allows each successive module to know how
 * many modules are in the chain before this one. As soon as the second byte is received, the module will execute
 * property read callback.
 *
 * \param[inout] ctx Pointer to the chain_comm_ctx_t structure containing the context information.
 */
void chain_comm_state_readAll_rxCnt(chain_comm_ctx_t *ctx)
{
    uint8_t data;
    if (uart_driver_cnt_writable(ctx->uart) && uart_driver_read(ctx->uart, &data, 1)) {
        ctx->data_cnt++;
        if (ctx->data_cnt == 1) {
            ctx->index = ((uint16_t)data) + 1;
            data       = (ctx->index >> 0) & 0xff;
        } else {
            ctx->index += (data << 8);
            data = (ctx->index >> 8) & 0xff;
            chain_comm_exec(ctx);
            if (--ctx->index) {
                chain_comm_state_change(ctx, rxSize);
            } else {
                if (chain_comm_property_read_attributes_get(ctx->header.property)->dynamic_property_size) {
                    chain_comm_state_change(ctx, readAll_txSize);
                } else {
                    chain_comm_state_change(ctx, readAll_txData);
                }
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
 * This state receives the data bytes written by the readAll operation of the previous module. These bytes are
 * forwarded to the next modules. Once all data of the previous modules has been forwarded, the module will change
 * the state to readAll_txData.
 *
 * \param[inout] ctx Pointer to the chain_comm_ctx_t structure containing the context information.
 */
void chain_comm_state_readAll_rxData(chain_comm_ctx_t *ctx)
{
    uint8_t data;

    while (uart_driver_cnt_writable(ctx->uart) && ctx->data_cnt < ctx->property_size &&
           uart_driver_read(ctx->uart, &data, 1)) {
        ctx->data_cnt++;
        uart_driver_write(ctx->uart, &data, 1);
    }
    if (ctx->data_cnt == ctx->property_size) {
        if (--ctx->index) {
            chain_comm_state_change(ctx, rxSize);
        } else {
            if (chain_comm_property_read_attributes_get(ctx->header.property)->dynamic_property_size) {
                chain_comm_state_change(ctx, readAll_txSize);
            } else {
                chain_comm_state_change(ctx, readAll_txData);
            }
        }
    } else if (chain_comm_timer_elapsed(ctx)) {
        chain_comm_state_change(ctx, rxHeader);
    }
}

/**
 * \brief Handles the readAll_txSize state of the chain communication FSM.
 *
 * This state sends the property size to the next module in the chain. Once the size has been sent, the state will
 * change to the txData state.
 *
 * \param[inout] ctx Pointer to the chain_comm_ctx_t structure containing the context information.
 */
void chain_comm_state_readAll_txSize(chain_comm_ctx_t *ctx)
{
    if (ctx->data_cnt == 0 && uart_driver_cnt_writable(ctx->uart) >= 2) {
        /* Send dynamic size. */
        uart_driver_write(ctx->uart, (uint8_t *)&ctx->property_size, 2);
        chain_comm_state_change(ctx, readAll_txData);
    } else if (chain_comm_timer_elapsed(ctx)) {
        chain_comm_state_change(ctx, rxHeader);
    }
}

/**
 * \brief Handles the readAll_txData state of the chain communication FSM.
 *
 * This state sends the data bytes of the readAll operation to the next module. Once all data has been sent, the
 * state will change to rxHeader.
 *
 * \param[inout] ctx Pointer to the chain_comm_ctx_t structure containing the context information.
 */
void chain_comm_state_readAll_txData(chain_comm_ctx_t *ctx)
{
    /* Send data. */
    uint8_t data_cnt =
        uart_driver_write(ctx->uart, &ctx->property_data[ctx->data_cnt], ctx->property_size - ctx->data_cnt);
    ctx->data_cnt += data_cnt;
    if (ctx->data_cnt == ctx->property_size) {
        chain_comm_state_change(ctx, rxHeader);
    } else if (chain_comm_timer_elapsed(ctx)) {
        chain_comm_state_change(ctx, rxHeader);
    }
}

/**
 * \brief Handles the writeAll_rxData state of the chain communication FSM.
 *
 * This state receives the data bytes to be written by the writeAll operation. All data is received is also passed
 * to the next module. Once all data has been forwarded to the next module, the module will execute the writeAll
 * command for the property and change the state to writeAll_rxAck.
 *
 * \param[inout] ctx Pointer to the chain_comm_ctx_t structure containing the context information.
 */
void chain_comm_state_writeAll_rxData(chain_comm_ctx_t *ctx)
{
    while (uart_driver_cnt_writable(ctx->uart) && ctx->data_cnt < ctx->property_size &&
           uart_driver_read(ctx->uart, &ctx->property_data[ctx->data_cnt], 1)) {
        if (!uart_driver_write(ctx->uart, &ctx->property_data[ctx->data_cnt], 1)) {
            debug_io_log_error("Error writing data to uart\n");
        };
        ctx->data_cnt++;
    }
    if (ctx->data_cnt == ctx->property_size &&
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
 */
void chain_comm_state_writeAll_rxAck(chain_comm_ctx_t *ctx)
{
    uint8_t data = 0;
    if (uart_driver_cnt_writable(ctx->uart) && uart_driver_read(ctx->uart, &data, 1)) {
        if (data == 0xff) {
            debug_io_log_debug("Ack received\n");
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
 * This state receives the data bytes to be written by the writeSequential operation. The received bytes are NOT
 * passed to sequential modules. Once the data required for the writeSequential operation has been received, the
 * state will change to writeSeq_rxToTx.
 *
 * \param[inout] ctx Pointer to the chain_comm_ctx_t structure containing the context information.
 */
void chain_comm_state_writeSeq_rxData(chain_comm_ctx_t *ctx)
{
    if (uart_driver_read(ctx->uart, &ctx->property_data[ctx->data_cnt], 1)) {
        if (++ctx->data_cnt == ctx->property_size) {
            chain_comm_state_change(ctx, writeSeq_rxToTx);
        }
    } else if (chain_comm_timer_elapsed(ctx)) {
        chain_comm_state_change(ctx, rxHeader);
    }
}

/**
 * \brief Handles the writeSeq_rxToTx state of the chain communication FSM.
 *
 * This state will forward all received data to the next module. This will happen until the timeout event occurs.
 * Once the timeout has occurred, the module will execute the writeSequential command for the property and change
 * the state to rxHeader.
 *
 * \param[inout] ctx Pointer to the chain_comm_ctx_t structure containing the context information.
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