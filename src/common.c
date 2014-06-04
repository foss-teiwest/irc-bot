#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/wait.h>
#include "socket.h"
#include "irc.h"
#include "common.h"

#define ALLOC_ERROR(function, file, line) exit_msg("Failed to allocate memory in %s() %s:%d", function, file, line);

void exit_msg(const char *format, ...) {

	va_list args;

	assert(format);
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	putc('\n', stderr);

	exit(EXIT_FAILURE);
}

bool streq(const char *s1, const char *s2) {

	return strcmp(s1, s2) == 0;
}

bool strcase_eq(const char *s1, const char *s2) {

	return strcasecmp(s1, s2) == 0;
}

bool starts_with(const char *s1, const char *s2) {

	return strncmp(s1, s2, strlen(s2)) == 0;
}

bool starts_case_with(const char *s1, const char *s2) {

	return strncasecmp(s1, s2, strlen(s2)) == 0;
}

void *_malloc_w(size_t size, const char *caller, const char *file, int line) {

	void *buffer;

	buffer = malloc(size);
	if (!buffer)
		ALLOC_ERROR(caller, file, line); // We exit here

	return buffer;
}

void *_calloc_w(size_t size, const char *caller, const char *file, int line) {

	void *buffer;

	buffer = calloc(1, size);
	if (!buffer)
		ALLOC_ERROR(caller, file, line);

	return buffer;
}

void *_realloc_w(void *buf, size_t size, const char *caller, const char *file, int line) {

	void *buffer;

	buffer = realloc(buf, size);
	if (!buffer) // Exit instead of returning the old memory back to the program
		ALLOC_ERROR(caller, file, line);

	return buffer;
}

bool null_terminate(char *buf, char delim) {

	char *test;

	if (!buf)
		return false;

	test = strchr(buf, delim);
	if (!test)
		return false;

	*test = '\0';
	return true;
}

char *trim_whitespace(char *str) {

	char *end;

	if (!str)
		return NULL;

	while (isspace(*str))
		str++;

	if (!*str)
		return NULL;

	end = str + strlen(str) - 1;
	while (end >= str && isspace(*end))
		end--;

	// Return NULL if only whitespace was found
	*++end = '\0';
	return str == end ? NULL : str;
}

int extract_params(char *msg, char **argv[]) {

	char *savedptr;
	int argc = 0, size = STARTING_PARAMS;

	*argv = NULL;
	if (!msg)
		return 0;

	// Allocate enough starting space for most bot commands
	*argv = malloc_w(size * sizeof(char *));

	// split parameters seperated by space or tab
	(*argv)[argc] = strtok_r(msg, " \t", &savedptr);
	while ((*argv)[argc]) {
		if (argc == size - 1) { // Double the array if it gets full
			*argv = realloc_w(*argv, size * 2 * sizeof(char *));
			size *= 2;
		}
		(*argv)[++argc] = strtok_r(NULL, " \t", &savedptr);
	}
	if (!argc) {
		free(*argv);
		*argv = NULL;
	}
	return argc;
}

int get_int(const char *num, int max) {

	long converted_num;

	converted_num = strtol(num, NULL, 10);
	if (converted_num >= max)
		return max;
	else if (converted_num <= 0)
		return 1;
	else
		return converted_num;
}

void send_all_lines(Irc server, const char *target, FILE *stream) {

	int len;
	char line[LINELEN];

	// Print line by line the output of the program
	while (fgets(line, LINELEN, stream)) {
		len = strlen(line);
		if (len > 2) { // Only print if line is not empty
			line[len - 1] = '\0'; // Remove last newline char (\n) since we add it inside send_message()
			send_message(server, target, "%s", line); // The %s is needed to avoid interpeting format specifiers in output
		}
	}
}

int print_cmd_output(Irc server, const char *target, char *cmd_args[]) {

	FILE *prog;
	int status, fd[RDWR];

	if (pipe(fd)) {
		perror("pipe");
		return EXIT_FAILURE;
	}
	switch (fork()) {
	case -1:
		perror("fork");
		return EXIT_FAILURE;
	case 0:
		close(fd[RD]); // Close reading end of the socket

		// Re-open stdout to point to the writting end of our socket
		if (dup2(fd[WR], STDOUT_FILENO) != STDOUT_FILENO) {
			perror("dup2");
			return EXIT_FAILURE;
		}
		close(fd[WR]); // We don't need this anymore
		execvp(cmd_args[0], cmd_args);

		perror("exec failed"); // Exec functions return only on error
		_exit(EXIT_FAILURE);
	}
	close(fd[WR]); // Close writting end

	// Open socket as FILE stream since we need to print in lines anyway
	prog = fdopen(fd[RD], "r");
	if (!prog)
		return EXIT_FAILURE;

	send_all_lines(server, target, prog);

	fclose(prog);
	wait(&status); // Don't leave zombie
	return WEXITSTATUS(status);
}

int print_cmd_output_unsafe(Irc server, const char *target, const char *cmd) {

	// Open the program with arguments specified
	FILE *prog = popen(cmd, "r");
	if (!prog)
		return EXIT_FAILURE;

	send_all_lines(server, target, prog);
	return WEXITSTATUS(pclose(prog));
}

char *iso8859_7_to_utf8(const char *iso) {

	unsigned char *uiso, *utf;
	unsigned i = 0, y = 0;

	uiso = (unsigned char *) iso;
	utf = malloc_w(strlen(iso) * 2);

	while (uiso[i] != '\0') {
		if (uiso[i] > 0xa0) {
			if (uiso[i] < 0xf0) {
				utf[y] = 0xce;
				utf[y + 1] = uiso[i] - 48;
			} else {
				utf[y] = 0xcf;
				utf[y + 1] = uiso[i] - 112;
			}
			y += 2;
		} else {
			utf[y] = uiso[i];
			y++;
		}
		i++;
	}
	utf[y] = '\0';
	return (char *) utf;
}
