#ifndef WRAPPERS_H
#define WRAPPERS_H

// Allocate memory and print the caller function on failure (before exiting)
#define malloc_w(x) _malloc_w((x), __func__)
void *_malloc_w(size_t size, const char *caller);

// Takes a format specifier with a variable number of arguments
// Prints message and exits with failure
void exit_msg(const char *format, ...);

#endif