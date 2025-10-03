#include "simple_term.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//======================================================================================================================
//                                                   GLOBAL VARIABLES
//======================================================================================================================

static simple_term_entry_t term_keywords[SIMPLE_TERM_KEYWORD_MAX] = {0};

//======================================================================================================================
//                                                  FUNCTION PROTOTYPES
//======================================================================================================================

static int simple_term_make_argv(char *input, char *argv[], int max_args);

//======================================================================================================================
//                                                   PUBLIC FUNCTIONS
//======================================================================================================================

void simple_term_init(void)
{
    memset(term_keywords, 0, sizeof(term_keywords));
}

//----------------------------------------------------------------------------------------------------------------------

void simple_term_process(void)
{
    static char input_buffer[SIMPLE_TERM_INPUT_MAX] = {0};
    static unsigned input_index                     = 0;

    uint8_t c = 0;

    while (read(0 /* STDIN */, &c, 1) == 1) {
        if (c == '\n') {
            input_buffer[input_index] = '\0'; // Null-terminate the string
            input_index               = 0;    // Reset index for next input

            /* Convert to args. */
            char *argv[SIMPLE_TERM_ARGC_MAX] = {0};
            int argc                         = simple_term_make_argv(input_buffer, argv, SIMPLE_TERM_ARGC_MAX);

            if (argc == 0) {
                return; // No command entered
            }

            for (int i = 0; i < SIMPLE_TERM_KEYWORD_MAX; i++) {
                if (term_keywords[i].keyword != NULL &&
                    strncmp(argv[0], term_keywords[i].keyword, strlen(argv[0])) == 0) {
                    term_keywords[i].callback(argc, argv, term_keywords[i].userdata);
                    return;
                }
            }
            printf("Unknown command: %s\n", input_buffer);
        } else if (c == '\b' || c == 0x7F) { // Handle backspace
            if (input_index > 0) {
                input_buffer[--input_index] = '\0'; // Null-terminate the string
                write(1 /* STDOUT */, "\b \b", 3);  // Send backspace to terminal
            }
        } else if (c == 0x1B) { // Handle escape sequence
            printf("Escape sequence detected, command ignored.\n");
            input_index = 0; // Reset index to prevent overflow
            return;
        } else if (input_index < sizeof(input_buffer) - 1) {
            input_buffer[input_index++] = c; // Store character in buffer
        } else {
            printf("Input buffer overflow, command ignored.\n");
            input_index = 0; // Reset index to prevent overflow
            return;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

void simple_term_register_keyword(const char *keyword, simple_term_callback_t cb, void *userdata)
{
    for (int i = 0; i < SIMPLE_TERM_KEYWORD_MAX; i++) {
        if (term_keywords[i].keyword == NULL) {
            term_keywords[i].keyword  = keyword;
            term_keywords[i].callback = cb;
            term_keywords[i].userdata = userdata;
            return;
        }
    }
}

//======================================================================================================================
//                                                         PRIVATE FUNCTIONS
//======================================================================================================================

static int simple_term_make_argv(char *input, char *argv[], int max_args)
{
    int argc = 0;
    char *p  = input;

    while (*p != '\0' && argc < max_args) {
        // Skip leading whitespace
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == '\0') {
            break;
        }

        // Start of argument
        argv[argc++] = p;

        // Find end of argument
        while (*p && *p != ' ' && *p != '\t') {
            p++;
        }

        // Null-terminate argument
        if (*p) {
            *p++ = '\0';
        }
    }
    return argc;
}