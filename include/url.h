#ifndef URL_H
#define URL_H

#define URLLEN 300

struct mem_buffer {
	char *buffer;
	size_t size;
};

// Send long_url to a shortener service and return the short version or NULL on failure
char *shorten_url(char *long_url, struct mem_buffer *mem);

#endif