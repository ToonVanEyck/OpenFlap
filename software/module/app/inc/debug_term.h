/**
 * @file debug_term.h
 *
 * The debug terminal allows for debugging and testing various functionalities of the OpenFlap module through an RTT
 * terminal.
 */

#include "openflap.h"
#include "simple_term.h"

/**
 * @brief Initializes the debug terminal and registers keywords.
 */
void debug_term_init(of_ctx_t *ctx);