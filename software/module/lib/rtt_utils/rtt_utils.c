#include "rtt_utils.h"

#include <stdio.h>

//======================================================================================================================
//                                                   PUBLIC FUNCTIONS
//======================================================================================================================

void rtt_init()
{
    SEGGER_RTT_Init();
}

//----------------------------------------------------------------------------------------------------------------------

void rtt_scope_init(char *scope_format)
{
    static char JS_RTT_UpBuffer[512] = {0};
    int ret = SEGGER_RTT_ConfigUpBuffer(RTT_BUFFER_SCOPE, scope_format, &JS_RTT_UpBuffer[0], sizeof(JS_RTT_UpBuffer),
                                        SEGGER_RTT_MODE_NO_BLOCK_SKIP);
    if (ret < 0) {
        printf("SEGGER_RTT_ConfigUpBuffer failed with error code %d\n", ret);
    }
}

//----------------------------------------------------------------------------------------------------------------------

void rtt_scope_push(void *datapoints, unsigned size)
{
    if (size > BUFFER_SIZE_UP) {
        printf("Data size exceeds buffer size.\n");
        return;
    }
    SEGGER_RTT_Write(RTT_BUFFER_SCOPE, datapoints, size);
}

//----------------------------------------------------------------------------------------------------------------------