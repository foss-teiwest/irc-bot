#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <yajl/yajl_tree.h>
#include "irc.h"
#include "twitter.h"

/**
 * @file common.h
 * Contains general functions that are needed by many files
 */

// Allow testing on static functions
#ifdef TEST
	#define STATIC
#else
	#define STATIC static
#endif

#define STARTSIZE   5
#define MAXQUOTES   20
#define PATHLEN     120
#define EXIT_MSGLEN 128
#define LINELEN     300
#define CONFSIZE    4096
#define TIMEOUT     300000 //!< Timeout in milliseconds for the poll function
#define LOCALHOST  "127.0.0.1"
#define SCRIPTDIR  "scripts/" //!< default folder to look for scripts like the youtube one
#define DEFAULT_CONFIG_NAME "config.json"

struct config_options {
	char *server;
	char *port;
	char *nick;
	char *nick_password;
	char *user;
	char *channels[MAXCHANS];
	int channels_set;
	char *bot_version;
	char *github_repo;
	char *quit_message;
	char *murmur_port;
	char *mpd_port;
	char *mpd_database;
	char *mpd_random_file;
	char *oauth_consumer_key;
	char *oauth_consumer_secret;
	char *oauth_token;
	char *oauth_token_secret;
	char *twitter_profile_url;
	char *twitter_access_list[MAXLIST];
	bool twitter_details_set;
	int access_list_count;
	char *quotes[MAXQUOTES];
	int quote_count;
	bool verbose;
};

extern struct config_options cfg; //!< global struct with config's values

/** Macros to help reduce boilerplate code */
#define CMD(...) (char *[]) { __VA_ARGS__, NULL }
#define CFG(...) (const char *[]) { __VA_ARGS__, NULL }
#define CFG_GET(struct_name, root, field) struct_name.field = get_json_field(root, #field)

//@{
/** Wrappers for allocating memory. Print the position on failure and exit. Will always return a valid pointer */
#define MALLOC_W(x) _malloc_w((x), __func__, __FILE__, __LINE__)
#define CALLOC_W(x) _calloc_w((x), __func__, __FILE__, __LINE__)
#define REALLOC_W(x, y) _realloc_w((x), (y), __func__, __FILE__, __LINE__)
#define ALLOC_ERROR(function, file, line)  exit_msg("Failed to allocate memory in %s() %s:%d", function, file, line);
//@}

void *_malloc_w(size_t size, const char *caller, const char *file, int line);
void *_calloc_w(size_t size, const char *caller, const char *file, int line);
void *_realloc_w(void *buf, size_t size, const char *caller, const char *file, int line);

/** Parse arguments, load config, install signal handlers etc
 *  @param argc, argv main's parameters unaltered */
void initialize(int argc, char *argv[]);

/** Cleanup curl, free yajl handler etc */
void cleanup(void);

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

