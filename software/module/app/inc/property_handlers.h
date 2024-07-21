#pragma once

#include "openflap.h"

/** Initialize property handlers. */
void property_handlers_init(openflap_ctx_t *ctx);

/** Store the configuration inb flash. */
void propertyHandlersConfigWrite(void);