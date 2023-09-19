#include "chain_comm.h"

uint8_t watch_tx = 0;

void chainCommRun(uint32_t *idle_timeout)
{
    static uint16_t comm_timer = 0; // timeout counter
    CLOCK_GUARD;
    (*idle_timeout)++;
    if (RX_DONE) {
        *idle_timeout = 0;
        comm_timer = 0;
        chainComm(dataAvailable);
    }
    if (watch_tx && TX_DONE) {
        *idle_timeout = 0;
        comm_timer = 0;
        watch_tx = 0;
        chainComm(transmitComplete);
    }
    if (comm_timer++ >= SEQUENTIAL_WRITE_TRIGGER_DELAY_MS * 10) {
        comm_timer = 0;
        chainComm(communicationTimeout);
    }
}

void chainComm(chainCommEvent_t event)
{
    static chainCommCtx_t ctx = {0};
    uint8_t data;
    switch (event) {
        case dataAvailable:
            TX_WAIT_DONE;
            RX_BYTE(data);
            ctx.cnt++;
            switch (ctx.state) {
                case receiveHeader:
                    // state logic
                    ctx.header.raw = data;
                    ctx.cnt = 0;
                    if (ctx.header.field.action == property_readAll || ctx.header.field.action == property_writeAll) {
                        TX_BYTE(data);
                    }
                    // state transitions
                    if (ctx.header.field.action == property_readAll) {
                        ctx.state = indexModules;
                    } else if (ctx.header.field.action == property_writeAll ||
                               ctx.header.field.action == property_writeSequential) {
                        if (propertySizes[ctx.header.field.property]) {
                            ctx.state = receiveData;
                        } else if (ctx.header.field.action == property_writeSequential) {
                            ctx.state = passthrough;
                        }
                    } else {
                        if (ctx.header.field.property) {
                            ctx.state = errorIgnoreData;
                        }
                    }
                    break;
                case indexModules:
                    // state logic
                    if (ctx.cnt == 1) {
                        ctx.index = data;
                        ctx.index++;
                        TX_BYTE(ctx.index & 0x00FF);
                    } else {
                        ctx.index += ((uint16_t)data << 8);
                        TX_BYTE((ctx.index >> 8) & 0x00FF);
                        if (propertyHandlers[ctx.header.field.property].get) {
                            propertyHandlers[ctx.header.field.property].get(ctx.data);
                        }
                        ctx.index--;
                    }
                    // state transitions
                    if (ctx.cnt == 2) {
                        ctx.cnt = 0;
                        if (ctx.index) {
                            ctx.state = passthrough;
                        } else {
                            ctx.state = transmitData;
                            TX_BYTE(ctx.data[ctx.cnt++]);
                            TX_WAIT_DONE;
                        }
                    }
                    break;
                case passthrough:
                    // state logic
                    if (ctx.cnt == propertySizes[ctx.header.field.property]) {
                        ctx.index--;
                        ctx.cnt = 0;
                    }
                    TX_BYTE(data);
                    // state transitions
                    if (ctx.header.field.action == property_readAll && !ctx.cnt && !ctx.index) {
                        ctx.state = transmitData;
                        TX_BYTE(ctx.data[ctx.cnt++]);
                        TX_WAIT_DONE;
                    }
                    break;
                case receiveData:
                    // state logic
                    ctx.data[ctx.cnt - 1] = data;
                    if (ctx.header.field.action == property_writeAll) {
                        TX_BYTE(data);
                        if (ctx.cnt == propertySizes[ctx.header.field.property]) {
                            if (propertyHandlers[ctx.header.field.property].set) {
                                propertyHandlers[ctx.header.field.property].set(ctx.data);
                            }
                            // TX_WAIT_DONE;
                            // TX_BYTE(0x00);
                        }
                    }
                    // state transitions
                    if (ctx.cnt == propertySizes[ctx.header.field.property]) {
                        if (ctx.header.field.action == property_writeAll) {
                            ctx.state = waitForAcknowledge;
                            // ctx.state = receiveHeader;
                            // ctx.index = 0;
                            // ctx.header.raw = 0;
                            // ctx.cnt = 0;
                        } else {
                            ctx.state = passthrough;
                        }
                    }
                    break;
                case waitForAcknowledge:
                    if (data == ACK) {
                        TX_BYTE(ACK);
                        // if(propertyHandlers[ctx.header.field.property].set){
                        //     propertyHandlers[ctx.header.field.property].set(ctx.data);
                        // }
                        ctx.state = receiveHeader;
                        ctx.index = 0;
                        ctx.header.raw = 0;
                        ctx.cnt = 0;
                    }
                    break;
                default:
                    break;
            }
        case transmitComplete:
            switch (ctx.state) {
                case transmitData:
                    // state logic
                    if (ctx.cnt < propertySizes[ctx.header.field.property]) {
                        TX_BYTE(ctx.data[ctx.cnt++]);
                        TX_WAIT_DONE;
                    }
                    // state transitions
                    if (ctx.cnt == propertySizes[ctx.header.field.property]) {
                        ctx.state = receiveHeader;
                        ctx.header.raw = 0;
                        ctx.cnt = 0;
                    }
                    break;
                default:
                    break;
            }
            break;
        case communicationTimeout:
            if (ctx.state == passthrough) {
                if (propertyHandlers[ctx.header.field.property].set) {
                    propertyHandlers[ctx.header.field.property].set(ctx.data);
                }
                ctx.state = waitForAcknowledge;
                // TX_WAIT_DONE;
                // TX_BYTE(0x00);
            } else {
                ctx.state = receiveHeader;
                ctx.index = 0;
                ctx.header.raw = 0;
                ctx.cnt = 0;
            }
            break;
    }
}
