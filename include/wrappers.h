#ifndef WRAPPERS_H
#define WRAPPERS_H

// Allocate memory with calloc (zeroing out) and print the caller function before exiting on fail
#define calloc_wrap(x) _calloc_wrap((x), __func__)
void *_calloc_wrap(size_t size, const char *caller);

// Takes a format specifier with a variable number of arguments
// Prints message and exits with failure
void exit_msg(const char *format, ...);

#endif