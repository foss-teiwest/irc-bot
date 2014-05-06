#ifndef CURL_H
#define CURL_H

/**
 * @file curl.h
 * Contains functions that use CURL directly
 */

#include <stddef.h>
#include <stdbool.h>
#include <yajl/yajl_tree.h>

#define URLLEN   440
#define TITLELEN 300

/** HTTP status codes */
enum http_codes {
	OK           = 200,
	UNAUTHORIZED = 401,
	FORBIDDEN    = 403
};

struct mem_buffer {
	char *buffer;
	size_t size;
};

struct github {
	char *sha;
	char *name;
	char *msg;
	char *url;
};

/**
 * Send long_url to google's shortener service and request a short version
 * @warning  Returned string must be freed when no longer needed
 *
 * @returns  short version or NULL on failure
 */
void *shorten_url(void *long_url_arg);

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
struct github *fetch_github_commits(yajl_val *root, const char *repo, int *commits);

/** Callback required by Curl if we want to save the output in a buffer
 *  @param membuf  Mem_buffer type is expected */
size_t curl_write_memory(char *data, size_t size, size_t elements, void *membuf);

/** Setup openssl for use in multi-threaded environment. Returns false if something went wrong */
bool openssl_crypto_init(void);

/** Cleanup the above steps */
void openssl_crypto_cleanup(void);

#endif

