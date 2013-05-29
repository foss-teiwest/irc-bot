#ifndef HELPER_H
#define HELPER_H

#include "irc.h"

#define STARTSIZE 5
#define LINELEN 300

// Allocate memory and print the caller function on failure (before exiting)
#define malloc_w(x) _malloc_w((x), __func__)
void *_malloc_w(size_t size, const char *caller);

#define realloc_w(x, y) _realloc_w((x), (y), __func__)
void *_realloc_w(void *buf, size_t size, const char *caller);

// Takes a format specifier with a variable number of arguments
// Prints message and exits with failure
void exit_msg(const char *format, ...);

// Split msg parameters seperated by space
// argc will contain the number of parameters that were successfully saved in the array returned
// e.x: argv = extract_params(pdata->message, &argc);  argv[argc - 1] will contain the last parameter
// The array returned must be freed.
char **extract_params(char *msg, int *argc);

// Run the program (and it's arguments) specified in cmd and print the result in dest
void print_cmd_output(Irc server, const char *dest, const char *cmd);

#endif