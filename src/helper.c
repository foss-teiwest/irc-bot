#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <yajl/yajl_tree.h>
#include "irc.h"
#include "helper.h"

extern pid_t main_pid;
yajl_val yajl_root;
struct config_options cfg;

void exit_msg(const char *format, ...) {

	char buf[128];
	va_list args;

	va_start(args, format);
	sprintf(buf, "%s\n", format);
	vfprintf(stderr, buf, args);
	va_end(args);

	if (getpid() == main_pid)
		exit(EXIT_FAILURE);
	else
		_exit(EXIT_FAILURE);
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

void print_cmd_output(Irc server, const char *dest, const char *cmd, char *args[]) {

	FILE *prog;
	char line[LINELEN];
	size_t len;
	int fd[2];

	if (pipe(fd) < 0) {
		perror("pipe");
		return;
	}

	switch (fork()) {
	case -1:
		perror("fork");
		return;
	case 0:
		close(fd[0]); // Close reading end of the socket

		// Re-open stdout to point to the writting end of our socket
		if (dup2(fd[1], STDOUT_FILENO) != STDOUT_FILENO) {
			perror("dup2");
			return;
		}
		close(fd[1]); // We don't need this anymore

		execvp(cmd, args);
		perror("exec failed"); // Exec functions return only on error
		return;
	}
	close(fd[1]); // Close writting end

	// Open socket as FILE stream since we need to print in lines anyway
	prog = fdopen(fd[0], "r");
	if (prog == NULL)
		return;

	// Print line by line the output of the program
	while (fgets(line, LINELEN, prog) != NULL) {
		len = strlen(line) - 1;
		if (len > 1) { // Only print if line is not empty
			line[len] = '\0'; // Remove last newline char (\n) since we add it inside send_message()
			send_message(server, dest, "%s", line); // The %s is needed to avoid interpeting format specifiers in output
		}
	}
	fclose(prog);
}

static size_t read_file(char **buf, const char *filename) {

	FILE *file;
	struct stat st;
	size_t n = 0;

	file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "fopen error: ");
		return 0;
	}

	if (fstat(fileno(file), &st) == -1) {
		fprintf(stderr, "fstat fail: ");
		goto cleanup;
	}

	if (st.st_size == 0 || st.st_size > CONFSIZE) {
		fprintf(stderr, "File too small/big: ");
		goto cleanup;
	}

	*buf = malloc_w(st.st_size + 1);
	n = fread(*buf, sizeof(char), st.st_size, file);
	if (n != (unsigned) st.st_size) {
		fprintf(stderr, "fread error: ");
		fclose(file);
		return 0;
	}
	(*buf)[st.st_size] = '\0';

cleanup:
	fclose(file);
	return n;
}

void parse_config(void) {

	char errbuf[1024], *buf = NULL;
	yajl_val val, array;
	int i;

	if (read_file(&buf, CONFPATH) == 0)
		exit_msg(CONFPATH);

	yajl_root = yajl_tree_parse(buf, errbuf, sizeof(errbuf));
	if (yajl_root == NULL)
		exit_msg("%s", errbuf);

	free(buf);

	if ((val   = yajl_tree_get(yajl_root, (const char *[]) { "server", NULL },   yajl_t_string)) == NULL) exit_msg("server: missing / wrong type");
	cfg.server = YAJL_GET_STRING(val);
	if ((val   = yajl_tree_get(yajl_root, (const char *[]) { "port", NULL },     yajl_t_number)) == NULL) exit_msg("port: missing / wrong type");
	cfg.port   = YAJL_GET_NUMBER(val);
	if ((val   = yajl_tree_get(yajl_root, (const char *[]) { "nick", NULL },     yajl_t_string)) == NULL) exit_msg("nick: missing / wrong type");
	cfg.nick   = YAJL_GET_STRING(val);
	if ((val   = yajl_tree_get(yajl_root, (const char *[]) { "user", NULL },     yajl_t_string)) == NULL) exit_msg("user: missing / wrong type");
	cfg.user   = YAJL_GET_STRING(val);

	if ((val        = yajl_tree_get(yajl_root, (const char *[]) { "nick_pwd", NULL },     yajl_t_string)) == NULL) exit_msg("nick_pwd: missing / wrong type");
	cfg.nick_pwd    = YAJL_GET_STRING(val);
	if ((val        = yajl_tree_get(yajl_root, (const char *[]) { "bot_version", NULL },  yajl_t_string)) == NULL) exit_msg("bot_version: missing / wrong type");
	cfg.bot_version = YAJL_GET_STRING(val);
	if ((val        = yajl_tree_get(yajl_root, (const char *[]) { "github_repo", NULL },  yajl_t_string)) == NULL) exit_msg("github_repo: missing / wrong type");
	cfg.github_repo = YAJL_GET_STRING(val);
	if ((val        = yajl_tree_get(yajl_root, (const char *[]) { "quit_message", NULL }, yajl_t_string)) == NULL) exit_msg("quit_message: missing / wrong type");
	cfg.quit_msg    = YAJL_GET_STRING(val);

	if ((val  = yajl_tree_get(yajl_root, (const char *[]) { "verbose", NULL }, yajl_t_any)) == NULL) exit_msg("verbose: missing");
	if (val->type != yajl_t_true && val->type != yajl_t_false) exit_msg("verbose: wrong type");
	cfg.verbose = YAJL_IS_TRUE(val);

	if ((array = yajl_tree_get(yajl_root, (const char *[]) { "channels", NULL }, yajl_t_array)) == NULL) exit_msg("channels: missing / wrong type");
	cfg.ch.channels_set = YAJL_GET_ARRAY(array)->len;

	if (cfg.ch.channels_set > MAXCHANS) {
		cfg.ch.channels_set = MAXCHANS;
		fprintf(stderr, "Channel limit reached (%d). Ignoring rest\n", MAXCHANS);
	}
	for (i = 0; i < cfg.ch.channels_set; i++) {
		val = YAJL_GET_ARRAY(array)->values[i];
		cfg.ch.channels[i] = YAJL_GET_STRING(val);
	}

	if ((array = yajl_tree_get(yajl_root, (const char *[]) { "fail_quotes", NULL }, yajl_t_array)) == NULL) exit_msg("fail_quotes: missing / wrong type");
	cfg.q.quote_count = YAJL_GET_ARRAY(array)->len;

	for (i = 0; i < cfg.q.quote_count; i++) {
		val = YAJL_GET_ARRAY(array)->values[i];
		cfg.q.quotes[i] = YAJL_GET_STRING(val);
	}
}

char *iso8859_7_to_utf8(char *iso) {

	unsigned char *uiso, *utf;
	unsigned int i = 0, y = 0;

	uiso = (unsigned char *) iso;
	utf = malloc_w(strlen(iso) * 2);

	while (uiso[i] != '\0') {
		if (uiso[i] > 0xa0) {
			if (uiso[i] < 0xf0) {
				utf[y] = 0xce;
				utf[y + 1] = uiso[i] - 48;
			}
			else {
				utf[y] = 0xcf;
				utf[y + 1] = uiso[i] - 112;
			}
			y += 2;
		}
		else {
			utf[y] = uiso[i];
			y++;
		}
		i++;
	}
	utf[y] = '\0';
	return (char *) utf;
}
