#pragma once

typedef enum log_lvl_tag {
    LOG_DISBALED  = 0,
    LOG_LVL_ERROR = 1,
    LOG_LVL_WARN  = 2,
    LOG_LVL_INFO  = 3,
    LOG_LVL_DEBUG = 4,
} log_lvl_t;

/**
 * \brief Callback type for terminal input processing.
 *
 * This callback is called when a complete line of input is received from the terminal.
 *
 * \param[in] input The input string after the keyword, received from the terminal.
 * \param[in] arg   Additional argument passed to the callback, can be used for context.
 */
typedef void (*debug_io_term_callback_t)(const char *input, void *arg);

/**
 * \brief Initialize the debug IO.
 *
 * \param[in] log_lvl The log level to be used.
 */
void debug_io_init(log_lvl_t log_lvl);

/**
 * \brief Initialize a scope.
 *
 * Supported formats:
 * b : 8 bit bitfield (1 byte)
 * f : float (4 bytes)
 * i : integer (1, 2 or 4 bytes)
 * u : unsigned integer (1, 2 or 4 bytes)
 *
 * \note for unsigned or signed integers, use i1, i2, i4 or u1, u2, u4 to specify the size.
 */
void debug_io_scope_init(char *scope_fomat);

/**
 * \brief Get a character from the debug IO.
 *
 * \return Negative if no character is available, otherwise returns the character.
 */
int debug_io_get(void);

/**
 * \brief Log a formatted \b debug message.
 *
 * \param[in] fmt Format string like printf.
 * \param[in] ... Arguments for the format.
 */
void debug_io_log_debug(const char *fmt, ...);

/**
 * \brief Log a formatted \b info message.
 *
 * \param[in] fmt Format string like printf.
 * \param[in] ... Arguments for the format.
 */
void debug_io_log_info(const char *fmt, ...);

/**
 * \brief Log a formatted \b warning message.
 *
 * \param[in] fmt Format string like printf.
 * \param[in] ... Arguments for the format.
 */
void debug_io_log_warn(const char *fmt, ...);

/**
 * \brief Log a formatted \b error message.
 *
 * \param[in] fmt Format string like printf.
 * \param[in] ... Arguments for the format.
 */
void debug_io_log_error(const char *fmt, ...);

/**
 * \brief Set the log level.
 *
 * \param[in] log_lvl The log level to be set.
 */
void debug_io_log_set_level(log_lvl_t log_lvl);

/**
 * \brief Get the log level.
 *
 * \return The current log level.
 */
log_lvl_t debug_io_log_get_level(void);

/**
 * \brief Enable the debug IO.
 * The last configured log level will be used.
 */
void debug_io_log_restore(void);

/**
 * \brief Process the terminal input.
 *
 * Reads characters from the debug input and processes them when an end-of-line character is received.
 * This function should be called periodically to handle incoming data.
 */
void debug_io_term_process(void);

/**
 * \brief Register a keyword for terminal input processing.
 *
 * This function allows you to register a keyword that, when typed in the terminal,
 * will trigger the associated callback function.
 *
 * \param[in] keyword The keyword to register. It should be a null-terminated string.
 * \param[in] cb      The callback function to be called when the keyword is detected.
 * @param[in] arg     Additional argument to be passed to the callback function.
 */
void debug_io_term_register_keyword(const char *keyword, debug_io_term_callback_t cb, void *arg);

/**
 * \brief Send data points to the debug IO scope.
 */
void debug_io_scope_push(void *datapoints, unsigned size);