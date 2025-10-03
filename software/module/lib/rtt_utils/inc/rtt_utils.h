#pragma once

#include "SEGGER_RTT.h"

#define RTT_BUFFER_DEFAULT 0
#define RTT_BUFFER_SCOPE   1

/**
 * \brief Initialize the debug IO.
 */
void rtt_init();

/**
 * \brief Initialize a scope.
 *
 * Supported formats:
 * b : 8 bit bitfield (1 byte)
 * f : float (4 bytes)
 * i : integer (1, 2 or 4 bytes)
 * u : unsigned integer (1, 2 or 4 bytes)
 * \param[in] scope_format The format string defining the scope data types.
 *
 * \note for unsigned or signed integers, use i1, i2, i4 or u1, u2, u4 to specify the size.
 */
void rtt_scope_init(char *scope_format);

/**
 * \brief Send data points to the debug IO scope.
 */
void rtt_scope_push(void *datapoints, unsigned size);