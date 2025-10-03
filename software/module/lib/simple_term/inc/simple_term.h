#pragma once

#define SIMPLE_TERM_KEYWORD_MAX 20
#define SIMPLE_TERM_ARGC_MAX    10
#define SIMPLE_TERM_INPUT_MAX   256

/**
 * \brief Callback type for terminal input processing.
 *
 * This callback is called when a complete line of input is received from the terminal.
 *
 * \param[in] args The argument count.
 * \param[in] argv The argument vector, an array of strings.
 * \param[in] userdata Userdata passed to the callback, can be used for context.
 */
typedef void (*simple_term_callback_t)(int argc, char *argv[], void *userdata);

/**
 * \brief Entry for a registered keyword in the simple terminal.
 */
typedef struct {
    const char *keyword;             /**< The keyword to register, a null-terminated string. */
    simple_term_callback_t callback; /**< The callback function to be called when the keyword is detected. */
    void *userdata;                  /**< Userdata passed to the callback, can be used for context. */
} simple_term_entry_t;

/**
 * \brief Initialize the simple terminal.
 */
void simple_term_init(void);

/**
 * \brief Process the terminal input.
 *
 * Reads characters from the simple input and processes them when an end-of-line character is received.
 * This function should be called periodically to handle incoming data.
 */
void simple_term_process(void);

/**
 * \brief Register a keyword for terminal input processing.
 *
 * This function allows you to register a keyword that, when typed in the terminal,
 * will trigger the associated callback function.
 *
 * \param[in] keyword The keyword to register. It should be a null-terminated string.
 * \param[in] cb      The callback function to be called when the keyword is detected.
 * \param[in] userdata Userdata passed to the callback, can be used for context.
 */
void simple_term_register_keyword(const char *keyword, simple_term_callback_t cb, void *userdata);