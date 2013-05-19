#ifndef HTTP_H
#define HTTP_H

#define URLLEN 300

struct mem_buffer {
	char *buffer;
	size_t size;
};

// Send long_url to a shortener service and return the short version or NULL on failure
// Returned string must be freed when no longer needed
char *shorten_url(char *long_url);

#endif