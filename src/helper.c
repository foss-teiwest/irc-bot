#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "helper.h"


void exit_msg(const char *format, ...) {

	char buf[128];
	va_list args;

	va_start(args, format);
	sprintf(buf, "%s\n", format);
	vfprintf(stderr, buf, args);
	va_end(args);

	exit(EXIT_FAILURE);
}

void *_malloc_w(size_t size, const char *caller) {

	void *buffer;

	buffer = malloc(size);
	if (buffer == NULL)
		exit_msg("Error: calloc failed in %s", caller);

	return buffer;
}

void *_realloc_w(void *buf, size_t size, const char *caller) {

	void *buffer;

	buffer = realloc(buf, size);
	if (buffer == NULL)
		exit_msg("Error: realloc failed in %s", caller);

	return buffer;
}

char **extract_params(char *msg, int *argc) {

	int size;
	char *temp, **argv;

	// Allocate enough starting space for most bot commands
	argv = malloc_w(STARTSIZE * sizeof(char *));
	size = STARTSIZE;
	*argc = 0;

	// Null terminate the the whole parameters line
	temp = strrchr(msg, '\r');
	if (temp == NULL)
		return argv;
	*temp = '\0';

	// split parameters seperated by space or tab
	argv[*argc] = strtok(msg, " \t");
	while (argv[*argc] != NULL) {
		if (*argc == size - 1) { // Double the array if it gets full
			argv = realloc_w(argv, size * 2 * sizeof(char *));
			size *= 2;
		}
		argv[++(*argc)] = strtok(NULL, " \t");
	}
	return argv;
}