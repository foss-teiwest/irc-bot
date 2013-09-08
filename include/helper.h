#ifndef HELPER_H
#define HELPER_H

#include <stdbool.h>
#include <yajl/yajl_tree.h>
#include "irc.h"

/**
 * @file helper.h
 * Contains general functions that are needed by many files
 */

#define STARTSIZE   5
#define MAXQUOTES   20
#define PATHLEN     120
#define EXIT_MSGLEN 128
#define LINELEN     300
#define CONFSIZE    4096
#define TIMEOUT     300000 //!< Timeout in milliseconds for the poll function
#define LOCALHOST "127.0.0.1"

struct config_options {
	char *server;
	char *port;
	char *nick;
	char *nick_pwd;
	char *user;
	char *channels[MAXCHANS];
	int channels_set;
	char *bot_version;
	char *github_repo;
	char *quit_msg;
	char *murmur_port;
	char *mpd_port;
	char *mpd_database;
	char *mpd_random_mode_file;
	char *quotes[MAXQUOTES];
	int quote_count;
	bool verbose;
};

extern struct config_options cfg; //!< global struct with config's values

/** Returns array size
 *  @warning  Must ONLY be used for local arrays (same scope) allocated in stack */
#define SIZE(x) (int) (sizeof(x) / sizeof(x[0]))

//@{
/** Wrappers for allocating memory. Print the caller function on failure and exit. Will always return a valid pointer */
#define malloc_w(x) _malloc_w((x), __func__)
#define calloc_w(x) _calloc_w((x), __func__)
#define realloc_w(x, y) _realloc_w((x), (y), __func__)
//@}

void *_malloc_w(size_t size, const char *caller);
void *_calloc_w(size_t size, const char *caller);
void *_realloc_w(void *buf, size_t size, const char *caller);

/** Parse arguments, load config, install signal handlers etc
 *  @param argc, argv main's parameters unaltered */
void initialize(int argc, char *argv[]);

/** Cleanup curl, free yajl handler etc */
void cleanup(void);

/** Takes a format specifier with a variable number of arguments. Prints message and exits with failure
 *  If the caller is not the main process then _exit() will be used to avoid,
 *  calling the functions registered with atexit(), flushing descriptors etc */
void exit_msg(const char *format, ...);

/**
 * Extract parameters seperated by space and put them in an array
 * Example: argv = extract_params(pdata.message, &argc);  argv[argc - 1] will contain the last parameter
 * @warning  The array returned must be freed.

 * @param msg   A string containing space seperated arguments. Spaces will be replaced by null terminators
 * @param argc  argc will contain the number of parameters that were successfully saved in the array returned or 0 if none found
 */
char **extract_params(char *msg, int *argc);

/**
 * Convert string to number
 *
 * @param msg  String containing numbers
 * @param max  Is the maximum number we want returned
 * @returns    A number between 1 and max. Values <=0 return 1 as well
 */
int get_int(const char *num, int max);

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

/** Parse json config_file and update cfg global struct with the values read */
void parse_config(yajl_val root, const char *config_file);

/** Convert string's encoding from ISO 8859-7 to UTF-8
 *  @warning  Return value must be freed to avoid memory leak */
char *iso8859_7_to_utf8(char *iso);

#endif
