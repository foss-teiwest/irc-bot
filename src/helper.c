#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "helper.h"
#include "irc.h"


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
		exit_msg("Error: malloc failed in %s", caller);

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
	*argc = 0;

	// Allocate enough starting space for most bot commands
	argv = malloc_w(STARTSIZE * sizeof(char *));
	size = STARTSIZE;

	// Make sure we have at least 1 parameter before proceeding
	if (msg == NULL)
		return NULL;

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

void print_cmd_output(Irc server, const char *dest, const char *cmd) {

	char line[LINELEN];
	FILE *prog;
	int len;

	// Open the program with arguments specified
	prog = popen(cmd, "r");
	if (prog == NULL)
		return;

	// Print line by line the output of the program
	while (fgets(line, LINELEN, prog) != NULL) {
		len = strlen(line) - 1;
		if (len > 1) { // Only print if line is not empty
			line[len] = '\0'; // Remove last newline char (\n) since we add it inside send_message()
			send_message(server, dest, "%s", line);
		}
	}
	pclose(prog);
}