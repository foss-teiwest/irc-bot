#ifndef HELPER_H
#define HELPER_H

#include <stdbool.h>
#include "irc.h"

#define STARTSIZE 5
#define MAXQUOTES 20
#define LINELEN   300
#define CONFSIZE  4096
#define CONFPATH "config.json"

struct config_options {
	char *server;
	char *port;
	char *nick;
	char *nick_pwd;
	char *user;
	struct {
		int channels_set;
		char *channels[MAXCHANS];
	} ch;
	char *bot_version;
	char *github_repo;
	char *quit_msg;
	struct {
		int quote_count;
		char *quotes[MAXQUOTES];
	} q;
	bool verbose;
};

extern struct config_options cfg;

// Returns array size. Warning: must ONLY be used for local arrays (same scope) allocated in stack
#define SIZE(x) (sizeof(x) / sizeof(x[0]))

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
// e.x: argv = extract_params(pdata.message, &argc);  argv[argc - 1] will contain the last parameter
// The array returned must be freed.
char **extract_params(char *msg, int *argc);

// Convert string to number. max is the maximum number returned and 1 for negative values
int get_int(const char *num, int max);

// Run the program (and it's arguments) specified in cmd and print the result in dest
void print_cmd_output(Irc server, const char *dest, const char *cmd, char *args[]);

// Parse json config and set values read to the cfg global struct
void parse_config(void);

// Convert string's encoding from ISO 8859-7 to UTF-8
// Return value must be freed to avoid memory leak
char *iso8859_7_to_utf8(char *iso);

#endif
