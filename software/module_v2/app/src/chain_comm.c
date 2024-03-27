#include "chain_comm.h"

uint8_t watch_tx = 0;
extern UART_HandleTypeDef UartHandle;

// void chainCommRun(uint32_t *idle_timeout)
// {
//     static uint16_t comm_timer = 0; // timeout counter
//     uint8_t rx_data;
//     // CLOCK_GUARD;
//     (*idle_timeout)++;
//     if (HAL_UART_Receive(UartHandle, &rx_data, 1, 0) == HAL_OK) {
//         *idle_timeout = 0;
//         comm_timer = 0;
//         chainComm(dataAvailable);
//     }
//     if (watch_tx && TX_DONE) {
//         *idle_timeout = 0;
//         comm_timer = 0;
//         watch_tx = 0;
//         chainComm(transmitComplete);
//     }
//     if (comm_timer++ >= SEQUENTIAL_WRITE_TRIGGER_DELAY_MS * 10) {
//         comm_timer = 0;
//         chainComm(communicationTimeout);
//     }
// }

// void chainComm(chainCommEvent_t event)
// {
//     static chainCommCtx_t ctx = {0};
//     uint8_t data;
//     switch (event) {
//         case dataAvailable:
//             TX_WAIT_DONE;
//             RX_BYTE(data);
//             ctx.cnt++;
//             switch (ctx.state) {
//                 case receiveHeader:
//                     // state logic
//                     ctx.header.raw = data;
//                     ctx.cnt = 0;
//                     if (ctx.header.field.action == property_readAll || ctx.header.field.action == property_writeAll)
//                     {
//                         TX_BYTE(data);
//                     }
//                     // state transitions
//                     if (ctx.header.field.action == property_readAll) {
//                         ctx.state = indexModules;
//                     } else if (ctx.header.field.action == property_writeAll ||
//                                ctx.header.field.action == property_writeSequential) {
//                         if (propertySizes[ctx.header.field.property]) {
//                             ctx.state = receiveData;
//                         } else if (ctx.header.field.action == property_writeSequential) {
//                             ctx.state = passthrough;
//                         }
//                     } else {
//                         if (ctx.header.field.property) {
//                             ctx.state = errorIgnoreData;
//                         }
//                     }
//                     break;
//                 case indexModules:
//                     // state logic
//                     if (ctx.cnt == 1) {
//                         ctx.index = data;
//                         ctx.index++;
//                         TX_BYTE(ctx.index & 0x00FF);
//                     } else {
//                         ctx.index += ((uint16_t)data << 8);
//                         TX_BYTE((ctx.index >> 8) & 0x00FF);
//                         if (propertyHandlers[ctx.header.field.property].get) {
//                             propertyHandlers[ctx.header.field.property].get(ctx.data);
//                         }
//                         ctx.index--;
//                     }
//                     // state transitions
//                     if (ctx.cnt == 2) {
//                         ctx.cnt = 0;
//                         if (ctx.index) {
//                             ctx.state = passthrough;
//                         } else {
//                             ctx.state = transmitData;
//                             TX_BYTE(ctx.data[ctx.cnt++]);
//                             TX_WAIT_DONE;
//                         }
//                     }
//                     break;
//                 case passthrough:
//                     // state logic
//                     if (ctx.cnt == propertySizes[ctx.header.field.property]) {
//                         ctx.index--;
//                         ctx.cnt = 0;
//                     }
//                     TX_BYTE(data);
//                     // state transitions
//                     if (ctx.header.field.action == property_readAll && !ctx.cnt && !ctx.index) {
//                         ctx.state = transmitData;
//                         TX_BYTE(ctx.data[ctx.cnt++]);
//                         TX_WAIT_DONE;
//                     }
//                     break;
//                 case receiveData:
//                     // state logic
//                     ctx.data[ctx.cnt - 1] = data;
//                     if (ctx.header.field.action == property_writeAll) {
//                         TX_BYTE(data);
//                         if (ctx.cnt == propertySizes[ctx.header.field.property]) {
//                             TX_WAIT_DONE;
//                             if (propertyHandlers[ctx.header.field.property].set) {
//                                 propertyHandlers[ctx.header.field.property].set(ctx.data);
//                             }
//                             // TX_BYTE(0x00);
//                         }
//                     }
//                     // state transitions
//                     if (ctx.cnt == propertySizes[ctx.header.field.property]) {
//                         if (ctx.header.field.action == property_writeAll) {
//                             ctx.state = waitForAcknowledge;
//                             // ctx.state = receiveHeader;
//                             // ctx.index = 0;
//                             // ctx.header.raw = 0;
//                             // ctx.cnt = 0;
//                         } else {
//                             ctx.state = passthrough;
//                         }
//                     }
//                     break;
//                 case waitForAcknowledge:
//                     if (data == ACK) {
//                         TX_BYTE(ACK);
//                         // if(propertyHandlers[ctx.header.field.property].set){
//                         //     propertyHandlers[ctx.header.field.property].set(ctx.data);
//                         // }
//                         ctx.state = receiveHeader;
//                         ctx.index = 0;
//                         ctx.header.raw = 0;
//                         ctx.cnt = 0;
//                     }
//                     break;
//                 default:
//                     break;
//             }
//         case transmitComplete:
//             switch (ctx.state) {
//                 case transmitData:
//                     // state logic
//                     if (ctx.cnt < propertySizes[ctx.header.field.property]) {
//                         TX_BYTE(ctx.data[ctx.cnt++]);
//                         TX_WAIT_DONE;
//                     }
//                     // state transitions
//                     if (ctx.cnt == propertySizes[ctx.header.field.property]) {
//                         ctx.state = receiveHeader;
//                         ctx.header.raw = 0;
//                         ctx.cnt = 0;
//                     }
//                     break;
//                 default:
//                     break;
//             }
//             break;
//         case communicationTimeout:
//             if (ctx.state == passthrough) {
//                 if (propertyHandlers[ctx.header.field.property].set) {
//                     propertyHandlers[ctx.header.field.property].set(ctx.data);
//                 }
//                 ctx.state = waitForAcknowledge;
//                 // TX_WAIT_DONE;
//                 // TX_BYTE(0x00);
//             } else {
//                 ctx.state = receiveHeader;
//                 ctx.index = 0;
//                 ctx.header.raw = 0;
//                 ctx.cnt = 0;
//             }
//             break;
//     }
// }

void chain_comm_state_change(chainCommCtx_v2_t *ctx, chainCommState_v2_t state)
{
    SEGGER_RTT_WriteString(0, chainCommStateNames[state]);
    SEGGER_RTT_WriteString(0, "\n");
    ctx->state = state;
    ctx->rx_cnt = 0;
    ctx->tx_cnt = 0;
}

/**
 * \brief Chain communication logic
 *
 * \param[inout] data On function entry: Data received. On function return: Data to be send.
 * \param[inout] size On function entry: Data received count. On function return: Data to be send count. Timeout event
 * if 0.
 */

bool chain_comm_state_rxHeader(chainCommCtx_v2_t *ctx, uint8_t *data, chainCommEvent_t event)
{
    switch (event) {
        case rx_event:
            ctx->index = 0;
            ctx->header.raw = *data;
            switch (ctx->header.field.action) {
                case property_readAll:
                    chain_comm_state_change(ctx, readAll_rxCnt);
                    break;
                case property_writeAll:
                    chain_comm_state_change(ctx, writeAll_rxData);
                    break;
                default:
                    chain_comm_state_change(ctx, rxHeader);
                    break;
            }
            return true;
        case tx_event:
            return false;
        case timeout_event:
            return false;
    }
    return false;
}

bool chain_comm_state_readAll_rxCnt(chainCommCtx_v2_t *ctx, uint8_t *data, chainCommEvent_t event)
{
    switch (event) {
        case rx_event:
            if (ctx->rx_cnt == 1) {
                ctx->index += ((uint16_t)*data) + 1;
                (*data)++;
            } else {
                ctx->index += (*data << 8);
                *data = (ctx->index >> 8) & 0xff;
                // DO READ CMD HERE
                if (--ctx->index) {
                    chain_comm_state_change(ctx, readAll_rxData);
                } else {
                    chain_comm_state_change(ctx, readAll_txData);
                }
            }
            return true;
        case tx_event:
            return false;
        case timeout_event:
            chain_comm_state_change(ctx, rxHeader);
            return false;
    }
    return false;
}

bool chain_comm_state_readAll_rxData(chainCommCtx_v2_t *ctx, uint8_t *data, chainCommEvent_t event)
{
    switch (event) {
        case rx_event:
            if (ctx->rx_cnt == propertySizes[ctx->header.field.property]) {
                if (--ctx->index) {
                    chain_comm_state_change(ctx, readAll_rxData);
                } else {
                    chain_comm_state_change(ctx, readAll_txData);
                }
            }
            return true;
        case tx_event:
            return false;
        case timeout_event:
            chain_comm_state_change(ctx, rxHeader);
            return false;
    }
    return false;
}

bool chain_comm_state_readAll_txData(chainCommCtx_v2_t *ctx, uint8_t *data, chainCommEvent_t event)
{
    switch (event) {
        case rx_event:
            // error
            chain_comm_state_change(ctx, rxHeader);
            return false;
        case tx_event:
            *data = ctx->property_data[ctx->tx_cnt - 1];
            if (ctx->tx_cnt == propertySizes[ctx->header.field.property]) {
                chain_comm_state_change(ctx, rxHeader);
            }
            return true;
        case timeout_event:
            chain_comm_state_change(ctx, rxHeader);
            return false;
    }
    return false;
}

bool chain_comm_state_writeAll_rxData(chainCommCtx_v2_t *ctx, uint8_t *data, chainCommEvent_t event)
{
    switch (event) {
        case rx_event:
            ctx->property_data[ctx->rx_cnt - 1] = *data;
            if (ctx->rx_cnt == propertySizes[ctx->header.field.property]) {
                chain_comm_state_change(ctx, writeAll_exec);
            }
            return true;
        case tx_event:
            return false;
        case timeout_event:
            chain_comm_state_change(ctx, rxHeader);
            return false;
    }
    return false;
}

bool chain_comm_state_writeAll_exec(chainCommCtx_v2_t *ctx, uint8_t *data, chainCommEvent_t event)
{
    switch (event) {
        case rx_event:
            return false;
        case tx_event:
            chain_comm_state_change(ctx, rxHeader);
            return false;
        case timeout_event:
            chain_comm_state_change(ctx, rxHeader);
            return false;
    }
    return false;
}

bool chain_comm_state_placeholder(chainCommCtx_v2_t *ctx, uint8_t *data, chainCommEvent_t event)
{
    switch (event) {
        case rx_event:

            break;
        case tx_event:

            break;
        case timeout_event:

            break;
    }
    return false;
}

bool chain_comm(chainCommCtx_v2_t *ctx, uint8_t *data, chainCommEvent_t event)
{
    if (event == rx_event) {
        ctx->rx_cnt++;
    } else if (event == tx_event) {
        ctx->tx_cnt++;
    }
    switch (ctx->state) {
        case rxHeader:
            return chain_comm_state_rxHeader(ctx, data, event);
        case readAll_rxCnt:
            return chain_comm_state_readAll_rxCnt(ctx, data, event);
        case readAll_rxData:
            return chain_comm_state_readAll_rxData(ctx, data, event);
        case readAll_txData:
            return chain_comm_state_readAll_txData(ctx, data, event);
        case writeAll_rxData:
            return chain_comm_state_writeAll_rxData(ctx, data, event);
        case writeAll_exec:
            return chain_comm_state_writeAll_exec(ctx, data, event);
        default:
            return false;
    }
}
