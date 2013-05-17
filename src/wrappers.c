#ifndef WRAPPERS_H
#define WRAPPERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "wrappers.h"

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

#endif