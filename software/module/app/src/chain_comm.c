#include "chain_comm.h"

void chain_comm_state_change(chain_comm_ctx_t *ctx, chain_comm_state_t state);
void chain_comm_exec(chain_comm_ctx_t *ctx);

/* Chain comm FSM state implementations. */
bool chain_comm_state_rxHeader(chain_comm_ctx_t *ctx, uint8_t *data, chain_comm_event_t event);
bool chain_comm_state_readAll_rxCnt(chain_comm_ctx_t *ctx, uint8_t *data, chain_comm_event_t event);
bool chain_comm_state_readAll_rxData(chain_comm_ctx_t *ctx, uint8_t *data, chain_comm_event_t event);
bool chain_comm_state_readAll_txData(chain_comm_ctx_t *ctx, uint8_t *data, chain_comm_event_t event);
bool chain_comm_state_writeAll_rxData(chain_comm_ctx_t *ctx, uint8_t *data, chain_comm_event_t event);
bool chain_comm_state_writeAll_rxAck(chain_comm_ctx_t *ctx, uint8_t *data, chain_comm_event_t event);
bool chain_comm_state_writeSeq_rxData(chain_comm_ctx_t *ctx, uint8_t *data, chain_comm_event_t event);
bool chain_comm_state_writeSeq_rxToTx(chain_comm_ctx_t *ctx, uint8_t *data, chain_comm_event_t event);

bool chain_comm(chain_comm_ctx_t *ctx, uint8_t *data, chain_comm_event_t event)
{
    if (event == rx_event) {
        ctx->rx_cnt++;
    } else if (event == tx_event) {
        ctx->tx_cnt++;
    }
    bool do_tx;
    switch (ctx->state) {
        case rxHeader:
            do_tx = chain_comm_state_rxHeader(ctx, data, event);
            break;
        case readAll_rxCnt:
            do_tx = chain_comm_state_readAll_rxCnt(ctx, data, event);
            break;
        case readAll_rxData:
            do_tx = chain_comm_state_readAll_rxData(ctx, data, event);
            break;
        case readAll_txData:
            do_tx = chain_comm_state_readAll_txData(ctx, data, event);
            break;
        case writeAll_rxData:
            do_tx = chain_comm_state_writeAll_rxData(ctx, data, event);
            break;
        case writeAll_rxAck:
            do_tx = chain_comm_state_writeAll_rxAck(ctx, data, event);
            break;
        case writeSeq_rxData:
            do_tx = chain_comm_state_writeSeq_rxData(ctx, data, event);
            break;
        case writeSeq_rxToTx:
            do_tx = chain_comm_state_writeSeq_rxToTx(ctx, data, event);
            break;
        default:
            do_tx = false;
            break;
    }
    return do_tx;
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
    debug_io_log_debug("State: %s -> %s\n", chain_comm_state_names[ctx->state], chain_comm_state_names[state]);
    ctx->state = state;
    ctx->rx_cnt = 0;
    ctx->tx_cnt = 0;
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
    if (ctx->header.field.action == property_readAll) {
        if (ctx->property_handler[ctx->header.field.property].get) {
            ctx->property_handler[ctx->header.field.property].get(ctx->property_data);
            debug_io_log_debug("Read %s property\n", propertyNames[ctx->header.field.property]);
        } else {
            debug_io_log_debug("Read %s property not supported\n", propertyNames[ctx->header.field.property]);
        }
    } else if (ctx->header.field.action == property_writeAll || ctx->header.field.action == property_writeSequential) {
        if (ctx->property_handler[ctx->header.field.property].set) {
            ctx->property_handler[ctx->header.field.property].set(ctx->property_data);
            debug_io_log_debug("Write %s property\n", propertyNames[ctx->header.field.property]);
        } else {
            debug_io_log_debug("Write %s property not supported\n", propertyNames[ctx->header.field.property]);
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
bool chain_comm_state_rxHeader(chain_comm_ctx_t *ctx, uint8_t *data, chain_comm_event_t event)
{
    bool do_tx = false;
    switch (event) {
        case rx_event:
            memset(ctx->property_data, 0, sizeof(ctx->property_data));
            ctx->index = 0;
            ctx->header.raw = *data;
            switch (ctx->header.field.action) {
                case property_readAll:
                    chain_comm_state_change(ctx, readAll_rxCnt);
                    do_tx = true;
                    break;
                case property_writeAll:
                    do_tx = true;
                    break;
                case property_writeSequential:
                    chain_comm_state_change(ctx, writeSeq_rxData);
                    break;
                case do_nothing:
                    if (ctx->header.field.property == no_property) {
                        do_tx = true; // Acknowledge
                    }
                    chain_comm_state_change(ctx, rxHeader);
                    break;
            }
        case tx_event:
            if (ctx->header.field.action == property_writeAll) {
                chain_comm_state_change(ctx, writeAll_rxData);
            }
            break;
        case timeout_event:
            break;
    }
    return do_tx;
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
bool chain_comm_state_readAll_rxCnt(chain_comm_ctx_t *ctx, uint8_t *data, chain_comm_event_t event)
{
    bool do_tx = false;
    switch (event) {
        case rx_event:
            if (ctx->rx_cnt == 1) {
                ctx->index += ((uint16_t)*data) + 1;
                *data = (ctx->index >> 0) & 0xff;
            } else {
                ctx->index += (*data << 8);
                *data = (ctx->index >> 8) & 0xff;
                chain_comm_exec(ctx);
                if (--ctx->index) {
                    chain_comm_state_change(ctx, readAll_rxData);
                } else {
                    chain_comm_state_change(ctx, readAll_txData);
                }
            }
            do_tx = true;
            break;
        case tx_event:
            break;
        case timeout_event:
            chain_comm_state_change(ctx, rxHeader);
            break;
    }
    return do_tx;
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
bool chain_comm_state_readAll_rxData(chain_comm_ctx_t *ctx, uint8_t *data, chain_comm_event_t event)
{
    bool do_tx = false;
    switch (event) {
        case rx_event:
            if (ctx->rx_cnt == propertySizes[ctx->header.field.property]) {
                if (--ctx->index) {
                    chain_comm_state_change(ctx, readAll_rxData);
                } else {
                    chain_comm_state_change(ctx, readAll_txData);
                }
            }
            do_tx = true;
            break;
        case tx_event:
            break;
        case timeout_event:
            chain_comm_state_change(ctx, rxHeader);
            break;
    }
    return do_tx;
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
bool chain_comm_state_readAll_txData(chain_comm_ctx_t *ctx, uint8_t *data, chain_comm_event_t event)
{
    bool do_tx = false;
    switch (event) {
        case rx_event:
            break;
        case tx_event:
            *data = ctx->property_data[ctx->tx_cnt - 1];
            if (ctx->tx_cnt == propertySizes[ctx->header.field.property]) {
                chain_comm_state_change(ctx, rxHeader);
            }
            do_tx = true;
            break;
        case timeout_event:
            chain_comm_state_change(ctx, rxHeader);
            break;
    }
    return do_tx;
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
bool chain_comm_state_writeAll_rxData(chain_comm_ctx_t *ctx, uint8_t *data, chain_comm_event_t event)
{
    bool do_tx = false;
    switch (event) {
        case rx_event:
            ctx->property_data[ctx->rx_cnt - 1] = *data;
            do_tx = true;
            break;
        case tx_event:
            if (ctx->tx_cnt == ctx->rx_cnt) {
                chain_comm_exec(ctx);
                chain_comm_state_change(ctx, writeAll_rxAck);
            }
            break;
        case timeout_event:
            chain_comm_state_change(ctx, rxHeader);
            break;
    }
    return do_tx;
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
bool chain_comm_state_writeAll_rxAck(chain_comm_ctx_t *ctx, uint8_t *data, chain_comm_event_t event)
{
    bool do_tx = false;
    switch (event) {
        case rx_event:
            do_tx = (*data == 0x00);
            chain_comm_state_change(ctx, rxHeader);
            break;
        case tx_event:
            break;
        case timeout_event:
            chain_comm_state_change(ctx, rxHeader);
            break;
    }
    return do_tx;
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
bool chain_comm_state_writeSeq_rxData(chain_comm_ctx_t *ctx, uint8_t *data, chain_comm_event_t event)
{
    bool do_tx = false;
    switch (event) {
        case rx_event:
            ctx->property_data[ctx->rx_cnt - 1] = *data;
            if (ctx->rx_cnt == propertySizes[ctx->header.field.property]) {
                chain_comm_state_change(ctx, writeSeq_rxToTx);
            }
            break;
        case tx_event:
            break;
        case timeout_event:
            chain_comm_state_change(ctx, rxHeader);
            break;
    }
    return do_tx;
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
bool chain_comm_state_writeSeq_rxToTx(chain_comm_ctx_t *ctx, uint8_t *data, chain_comm_event_t event)
{
    bool do_tx = false;
    switch (event) {
        case rx_event:
            do_tx = true;
            break;
        case tx_event:
            break;
        case timeout_event:
            chain_comm_exec(ctx);
            chain_comm_state_change(ctx, rxHeader);
            break;
    }
    return do_tx;
}