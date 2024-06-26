#pragma once

/**
 * \brief Initialize the debug IO.
 */
void debug_io_init(void);

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
