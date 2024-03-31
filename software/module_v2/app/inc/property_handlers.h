#pragma once

#include "openflap.h"

/** Initialize property handlers. */
void propertyHandlersInit(openflap_ctx_t *ctx);

/** Store the configuration inb flash. */
void propertyHandlersConfigWrite(void);