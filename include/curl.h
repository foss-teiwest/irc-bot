#ifndef CURL_H
#define CURL_H

#define URLLEN 440
#define COMMITLEN 180
#define TITLELEN 300

struct mem_buffer {
	char *buffer;
	size_t size;
};

typedef struct {
	char *sha;
	char *author;
	char *msg;
	char *url;
} Github;

// Send long_url to a shortener service and return the short version or NULL on failure
// Returned string must be freed when no longer needed
char *shorten_url(const char *long_url);

// Returns url title
char *get_url_title(const char *url);

// Returns mumble user list
char *fetch_mumble_users(void);

// Returns an array containing commits. The number of commits actually read, are stored in "commits"
Github *fetch_github_commits(const char *repo, int *commits, struct mem_buffer *mem);

#endif