#include "SEGGER_RTT.h"
#include "debug_io.h"
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

static bool format_ended_in_newline = true;

static inline bool format_end_in_newline(const char *fmt)
{
    return fmt[strlen(fmt) - 1] == '\n';
}

void debug_io_init(void)
{
    SEGGER_RTT_Init();
}

int debug_io_get(void)
{
    return SEGGER_RTT_GetKey();
}

void debug_io_log_debug(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (format_ended_in_newline) {
        SEGGER_RTT_printf(0, RTT_CTRL_TEXT_WHITE "D " RTT_CTRL_RESET);
    }
    format_ended_in_newline = format_end_in_newline(fmt);
    SEGGER_RTT_vprintf(0, fmt, &args);
    va_end(args);
}
void debug_io_log_info(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (format_ended_in_newline) {
        SEGGER_RTT_printf(0, RTT_CTRL_TEXT_BRIGHT_CYAN "I " RTT_CTRL_RESET);
    }
    format_ended_in_newline = format_end_in_newline(fmt);
    SEGGER_RTT_vprintf(0, fmt, &args);
    va_end(args);
}
void debug_io_log_warn(const char *fmt, ...)
{

    va_list args;
    va_start(args, fmt);
    if (format_ended_in_newline) {
        SEGGER_RTT_printf(0, RTT_CTRL_TEXT_BRIGHT_YELLOW "W " RTT_CTRL_RESET);
    }
    format_ended_in_newline = format_end_in_newline(fmt);
    SEGGER_RTT_vprintf(0, fmt, &args);
    va_end(args);
}
void debug_io_log_error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (format_ended_in_newline) {
        SEGGER_RTT_printf(0, RTT_CTRL_TEXT_BRIGHT_RED "E " RTT_CTRL_RESET);
    }
    format_ended_in_newline = format_end_in_newline(fmt);
    SEGGER_RTT_vprintf(0, fmt, &args);
    va_end(args);
}
