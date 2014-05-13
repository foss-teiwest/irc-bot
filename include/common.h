#ifndef COMMON_H
#define COMMON_H

/**
 * @file common.h
 * Contains general functions that are needed by many files
 */

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include "irc.h"

// Allow testing on static functions
#ifdef TEST
	#define STATIC
#else
	#define STATIC static
#endif

#define STARTING_PARAMS 5
#define LINELEN    350
#define MILLISECS  1000
#define MICROSECS (1000 * MILLISECS)
#define NANOSECS  (1000 * MICROSECS)
#define SCRIPTDIR "scripts/" //!< default folder to look for scripts like the youtube one

//@{
/** Macros to help reduce boilerplate code */
#define CMD(...) (char *[]) {__VA_ARGS__, NULL}
#define CFG(...) (const char *[]) {__VA_ARGS__, NULL}
//@}

//@{
/** Atomically fetch and set a boolean value (or char) using compiler builtins */
#define FETCH(x) __atomic_load_n      (&x, __ATOMIC_SEQ_CST)
#define TRUE(x)  __atomic_test_and_set(&x, __ATOMIC_SEQ_CST)
#define FALSE(x) __atomic_clear       (&x, __ATOMIC_SEQ_CST)
//@}

/** Find minimum number. Beware of side effects. */
#define MIN(a, b) ((a) < (b) ? (a) : (b))

//@{
/** Wrappers for allocating memory. On failure, print the position and exit. A valid pointer is returned always. */
#define malloc_w(x)     _malloc_w((x),       __func__, __FILE__, __LINE__)
#define calloc_w(x)     _calloc_w((x),       __func__, __FILE__, __LINE__)
#define realloc_w(x, y) _realloc_w((x), (y), __func__, __FILE__, __LINE__)
//@}

void *_malloc_w(size_t size, const char *caller, const char *file, int line);
void *_calloc_w(size_t size, const char *caller, const char *file, int line);
void *_realloc_w(void *buf, size_t size, const char *caller, const char *file, int line);

/** Takes a format specifier with a variable number of arguments. Prints message and exits with failure
 *  If the caller is not the main process then _exit() will be used to avoid,
 *  calling the functions registered with atexit(), flushing descriptors etc */
void exit_msg(const char *format, ...);

/** @returns  true if strings match */
bool streq(const char *s1, const char *s2);

/** @returns  true if s1 starts with s2 */
bool starts_with(const char *s1, const char *s2);

/** Case insensitive version */
bool starts_case_with(const char *s1, const char *s2);

/** Terminate buffer on the delim character
 *  @returns false if something went wrong */
bool null_terminate(char *buf, char delim);

/** Remove trailing whitespace
 *  @warning  The string is edited in-place so DON'T pass a read-only pointer */
bool trim_trailing_whitespace(char *str);

/**
 * Extract parameters seperated by space and put them in an array
 * Example: argc = extract_params(pdata.message, &argv);  argv[argc - 1] will contain the last parameter
 * @warning  The array returned must be freed.

 * @param msg   A string containing space seperated arguments. Spaces will be replaced by null terminators
 * @param argv  Pointer to array that parameters will be saved. Memory for the array will be allocated inside the function
 * @returns     The number of parameters that were successfully saved in the array (0 if none found)
 */
int extract_params(char *msg, char **argv[]);

/**
 * Convert string to number
 *
 * @param msg  String containing numbers
 * @param max  Is the maximum number we want returned
 * @returns    A number between 1 and max. Values <=0 return 1 as well
 */
int get_int(const char *num, int max);

/** Read all lines from stream and send them to server target */
void send_all_lines(Irc server, const char *target, FILE *stream);

/**
 * Run the program specified and print the output in target channel / person
 *
 * @param cmd_args  Is an array with it's first member having the actual program name,
 *                  the arguments come after that and the last member should be NULL
 *
 * Compound literals are an easy way to deal with that. Example (char *[]) { "ping", "-c", 3, google.com, NULL }
 */
void print_cmd_output(Irc server, const char *target, char *cmd_args[]);

/** Same as the above function but commands are interpeted by the shell so it is prone to exploits.
 *  @warning  Do NOT trust user input. Use only fixed values like "ls | wc -l" */
void print_cmd_output_unsafe(Irc server, const char *target, const char *cmd);

/** Convert string's encoding from ISO 8859-7 to UTF-8
 *  @warning  Return value must be freed to avoid memory leak */
char *iso8859_7_to_utf8(const char *iso);

#endif

