#pragma once

typedef enum log_lvl_tag {
    LOG_DISBALED  = 0,
    LOG_LVL_ERROR = 1,
    LOG_LVL_WARN  = 2,
    LOG_LVL_INFO  = 3,
    LOG_LVL_DEBUG = 4,
} log_lvl_t;

/**
 * \brief Initialize the debug IO.
 *
 * \param[in] log_lvl The log level to be used.
 */
void debug_io_init(log_lvl_t log_lvl);

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
 * \brief Disable the debug IO.
 */
void debug_io_log_disable(void);

/**
 * \brief Enable the debug IO.
 * The last configured log level will be used.
 */
void debug_io_log_enable(void);