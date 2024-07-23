#include "SEGGER_RTT.h"
#include "debug_io.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static bool format_ended_in_newline = true;
static log_lvl_t active_log_lvl = LOG_DISBALED;
static log_lvl_t prev_log_lvl = LOG_DISBALED;

static inline bool format_end_in_newline(const char *fmt)
{
    return (fmt[0] != 0) && (fmt[strlen(fmt) - 1] == '\n');
}

void debug_io_init(log_lvl_t log_lvl)
{
    active_log_lvl = log_lvl;
    prev_log_lvl = log_lvl;
    SEGGER_RTT_Init();
}

int debug_io_get(void)
{
    return SEGGER_RTT_GetKey();
}

void debug_io_log_debug(const char *fmt, ...)
{
    if (active_log_lvl < LOG_LVL_DEBUG) {
        return;
    }
    if (format_ended_in_newline) {
        SEGGER_RTT_Write(0, RTT_CTRL_TEXT_WHITE "D " RTT_CTRL_RESET, 14);
    }
    format_ended_in_newline = format_end_in_newline(fmt);
    va_list args;
    va_start(args, fmt);
    SEGGER_RTT_vprintf(0, fmt, &args);
    va_end(args);
}
void debug_io_log_info(const char *fmt, ...)
{
    if (active_log_lvl < LOG_LVL_INFO) {
        return;
    }
    if (format_ended_in_newline) {
        SEGGER_RTT_Write(0, RTT_CTRL_TEXT_BRIGHT_CYAN "I " RTT_CTRL_RESET, 14);
    }
    format_ended_in_newline = format_end_in_newline(fmt);
    va_list args;
    va_start(args, fmt);
    SEGGER_RTT_vprintf(0, fmt, &args);
    va_end(args);
}
void debug_io_log_warn(const char *fmt, ...)
{
    if (active_log_lvl < LOG_LVL_WARN) {
        return;
    }
    if (format_ended_in_newline) {
        SEGGER_RTT_Write(0, RTT_CTRL_TEXT_BRIGHT_YELLOW "W " RTT_CTRL_RESET, 14);
    }
    format_ended_in_newline = format_end_in_newline(fmt);
    va_list args;
    va_start(args, fmt);
    SEGGER_RTT_vprintf(0, fmt, &args);
    va_end(args);
}
void debug_io_log_error(const char *fmt, ...)
{
    if (active_log_lvl < LOG_LVL_ERROR) {
        return;
    }
    if (format_ended_in_newline) {
        SEGGER_RTT_Write(0, RTT_CTRL_TEXT_BRIGHT_RED "E " RTT_CTRL_RESET, 14);
    }
    format_ended_in_newline = format_end_in_newline(fmt);
    va_list args;
    va_start(args, fmt);
    SEGGER_RTT_vprintf(0, fmt, &args);
    va_end(args);
}

void debug_io_log_set_level(log_lvl_t log_lvl)
{
    active_log_lvl = log_lvl;
    prev_log_lvl = log_lvl;
}

log_lvl_t debug_io_log_get_level(void)
{
    return active_log_lvl;
}

void debug_io_log_disable(void)
{
    active_log_lvl = LOG_DISBALED;
}

void debug_io_log_enable(void)
{
    active_log_lvl = prev_log_lvl;
}