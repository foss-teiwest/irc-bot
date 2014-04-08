#ifndef CURL_H
#define CURL_H

/**
 * @file curl.h
 * Contains functions that use CURL directly
 */

#include <sys/types.h>
#include <yajl/yajl_tree.h>

#define URLLEN   440
#define TITLELEN 300

/** HTTP status codes */
enum http_codes {
	UNKNOWN      = 0,
	UNAUTHORIZED = 401,
	FORBIDDEN    = 403
};

typedef struct {
	char *buffer;
	size_t size;
} Mem_buffer;

typedef struct {
	char *sha;
	char *name;
	char *msg;
	char *url;
} Github;

/**
 * Send long_url to google's shortener service and request a short version
 * @warning  Returned string must be freed when no longer needed
 *
 * @returns  short version or NULL on failure
 */
char *shorten_url(const char *long_url);

/**
 * Get url's html and search for the title tag. Conversion from iso8859_7_to_utf8 will be used if needed
 * @warning  Returned string must be freed when no longer needed
 *
 * @returns  site's title or NULL on failure
 */
char *get_url_title(const char *url);

/** Returns mumble user list */
char *fetch_mumble_users(void);

/**
 * Interact with Github's api to get commit information
 * Returned struct must be freed when no longer needed
 * @warning Github struct's member are pointing to yajl_val root. Only free root when commit info is no longer needed
 *
 * @param repo     The repo to query in author/repo format
 * @param commits  The number of commits to return
 * @param root     yajl handle to store the json reply
 * @returns        An array of commits and maybe NULL on failure. commits will be updated with the actual number returned or 0 for error
 */
Github *fetch_github_commits(yajl_val *root, const char *repo, int *commits);

#endif

