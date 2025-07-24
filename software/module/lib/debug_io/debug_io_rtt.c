#include "SEGGER_RTT.h"
#include "debug_io.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define DEBUG_IO_TERM_KEYWORD_MAX 10

#define DEBUG_IO_BUFFER_TERMINAL (0) /* The RTT up buffer used by the terminal. */
#define DEBUG_IO_BUFFER_SCOPE    (1) /* The RTT up buffer used by the scope. */

static bool format_ended_in_newline = true;
static log_lvl_t active_log_lvl     = LOG_DISBALED;
static log_lvl_t prev_log_lvl       = LOG_DISBALED;

typedef struct {
    const char *keyword;
    debug_io_term_callback_t callback;
    void *arg; // Additional argument for the callback, can be used for context
} debug_io_term_entry_t;

static debug_io_term_entry_t term_keywords[DEBUG_IO_TERM_KEYWORD_MAX] = {0};

static inline bool format_end_in_newline(const char *fmt)
{
    return (fmt[0] != 0) && (fmt[strlen(fmt) - 1] == '\n');
}

void debug_io_init(log_lvl_t log_lvl)
{
    active_log_lvl = log_lvl;
    prev_log_lvl   = log_lvl;
    SEGGER_RTT_Init();
}

void debug_io_scope_init(char *scope_format)
{
    static char JS_RTT_UpBuffer[512] = {0};
    int ret = SEGGER_RTT_ConfigUpBuffer(DEBUG_IO_BUFFER_SCOPE, scope_format, &JS_RTT_UpBuffer[0],
                                        sizeof(JS_RTT_UpBuffer), SEGGER_RTT_MODE_NO_BLOCK_SKIP);
    if (ret < 0) {
        debug_io_log_error("SEGGER_RTT_ConfigUpBuffer failed with error code %d\n", ret);
    }
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
        SEGGER_RTT_Write(DEBUG_IO_BUFFER_TERMINAL, RTT_CTRL_TEXT_WHITE "D " RTT_CTRL_RESET, 14);
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
        SEGGER_RTT_Write(DEBUG_IO_BUFFER_TERMINAL, RTT_CTRL_TEXT_BRIGHT_CYAN "I " RTT_CTRL_RESET, 14);
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
        SEGGER_RTT_Write(DEBUG_IO_BUFFER_TERMINAL, RTT_CTRL_TEXT_BRIGHT_YELLOW "W " RTT_CTRL_RESET, 14);
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
        SEGGER_RTT_Write(DEBUG_IO_BUFFER_TERMINAL, RTT_CTRL_TEXT_BRIGHT_RED "E " RTT_CTRL_RESET, 14);
    }
    format_ended_in_newline = format_end_in_newline(fmt);
    va_list args;
    va_start(args, fmt);
    SEGGER_RTT_vprintf(0, fmt, &args);
    va_end(args);
}

void debug_io_log_set_level(log_lvl_t log_lvl)
{
    prev_log_lvl   = active_log_lvl;
    active_log_lvl = log_lvl;
}

log_lvl_t debug_io_log_get_level(void)
{
    return active_log_lvl;
}

void debug_io_log_restore(void)
{
    active_log_lvl = prev_log_lvl;
}

void debug_io_term_process(void)
{
    static char input_buffer[256] = {0};
    static unsigned input_index   = 0;

    int c = SEGGER_RTT_GetKey();

    if (c <= 0) {
        return; // No input available
    } else if (c == '\n') {
        input_buffer[input_index] = '\0'; // Null-terminate the string
        input_index               = 0;    // Reset index for next input
        char *space_pos           = strchr(input_buffer, ' ');
        size_t keyword_len        = space_pos ? (size_t)(space_pos - input_buffer) : strlen(input_buffer);
        const char *arg_str       = space_pos ? space_pos + 1 : NULL;

        for (int i = 0; i < DEBUG_IO_TERM_KEYWORD_MAX; i++) {
            if (term_keywords[i].keyword != NULL && strncmp(input_buffer, term_keywords[i].keyword, keyword_len) == 0 &&
                term_keywords[i].keyword[keyword_len] == '\0') {
                term_keywords[i].callback(arg_str, term_keywords[i].arg);
                return;
            }
        }
        debug_io_log_warn("Unknown command: %s\n", input_buffer);
    } else if (c == '\b' || c == 0x7F) { // Handle backspace
        if (input_index > 0) {
            input_index--;                    // Move back in the buffer
            input_buffer[input_index] = '\0'; // Null-terminate the string
            SEGGER_RTT_PutChar(0, '\b');      // Send backspace to terminal
            SEGGER_RTT_PutChar(0, ' ');       // Send space to terminal
            SEGGER_RTT_PutChar(0, '\b');      // Send backspace to terminal
        }
    } else if (c == 0x1B) { // Handle escape sequence
        debug_io_log_warn("Escape sequence detected, command ignored.\n");
        input_index = 0; // Reset index to prevent overflow
        return;
    } else if (input_index < sizeof(input_buffer) - 1) {
        input_buffer[input_index++] = c; // Store character in buffer
    } else {
        debug_io_log_error("Input buffer overflow, command ignored.\n");
        input_index = 0; // Reset index to prevent overflow
        return;
    }
}

void debug_io_term_register_keyword(const char *keyword, debug_io_term_callback_t cb, void *arg)
{
    for (int i = 0; i < DEBUG_IO_TERM_KEYWORD_MAX; i++) {
        if (term_keywords[i].keyword == NULL) {
            term_keywords[i].keyword  = keyword;
            term_keywords[i].callback = cb;
            term_keywords[i].arg      = arg;
            return;
        }
    }
}

void debug_io_scope_push(void *datapoints, unsigned size)
{
    if (size > BUFFER_SIZE_UP) {
        debug_io_log_error("Data size exceeds buffer size.\n");
        return;
    }
    SEGGER_RTT_Write(DEBUG_IO_BUFFER_SCOPE, datapoints, size);
}