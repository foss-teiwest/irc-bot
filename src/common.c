#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <yajl/yajl_tree.h>
#include "irc.h"
#include "mpd.h"
#include "twitter.h"
#include "common.h"

pid_t main_pid;
yajl_val root;
struct config_options cfg;
struct mpd_status_type *mpd_status;

void initialize(int argc, char *argv[]) {

	main_pid = getpid(); // store our process id to help exit_msg function exit appropriately

	// Accept config path as an optional argument
	if (argc > 2)
		exit_msg("Usage: %s [path_to_config]\n", argv[0]);
	else if (argc == 2)
		parse_config(root, argv[1]);
	else
		parse_config(root, "config.json");

	signal(SIGCHLD, SIG_IGN); // Make child processes not leave zombies behind when killed
	signal(SIGPIPE, SIG_IGN); // Handle writing on closed sockets on our own
	curl_global_init(CURL_GLOBAL_ALL); // Initialize curl library

	mpd_status = mmap(NULL, sizeof(*mpd_status), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (mpd_status == MAP_FAILED)
		perror("mmap");

	mpd_status->random = OFF;
	mpd_status->announce = OFF;
	if (!access(cfg.mpd_random_file, F_OK))
		mpd_status->random = ON;
}

void cleanup(void) {

	yajl_tree_free(root);
	curl_global_cleanup();
}

bool streq(const char *s1, const char *s2) {

	return strcmp(s1, s2) == 0;
}

bool starts_with(const char *s1, const char *s2) {

	return strncmp(s1, s2, strlen(s2)) == 0;
}

bool starts_case_with(const char *s1, const char *s2) {

	return strncasecmp(s1, s2, strlen(s2)) == 0;
}

void exit_msg(const char *format, ...) {

	char buf[EXIT_MSGLEN];
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

void *_malloc_w(size_t size, const char *caller, const char *file, int line) {

	void *buffer;

	buffer = malloc(size);
	if (!buffer)
		alloc_error(caller, file, line); // We exit here

	return buffer;
}

void *_calloc_w(size_t size, const char *caller, const char *file, int line) {

	void *buffer;

	buffer = calloc(1, size);
	if (!buffer)
		alloc_error(caller, file, line);

	return buffer;
}

void *_realloc_w(void *buf, size_t size, const char *caller, const char *file, int line) {

	void *buffer;

	buffer = realloc(buf, size);
	if (!buffer)
		alloc_error(caller, file, line);

	return buffer;
}

int extract_params(char *msg, char **argv[]) {

	int size, argc;
	char *temp;

	// Make sure we have at least 1 parameter before proceeding
	if (!msg)
		return 0;

	// Allocate enough starting space for most bot commands
	*argv = malloc_w(STARTSIZE * sizeof(char *));
	size = STARTSIZE;

	// Null terminate the the whole parameters line
	temp = strrchr(msg, '\r');
	if (!temp)
		return 0;

	*temp = '\0';
	argc = 0;

	// split parameters seperated by space or tab
	(*argv)[argc] = strtok(msg, " \t");
	while ((*argv)[argc]) {
		if (argc == size - 1) { // Double the array if it gets full
			*argv = realloc_w(*argv, size * 2 * sizeof(char *));
			size *= 2;
		}
		(*argv)[++argc] = strtok(NULL, " \t");
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

void print_cmd_output(Irc server, const char *target, char *cmd_args[]) {

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
		execvp(cmd_args[0], cmd_args);

		perror("exec failed"); // Exec functions return only on error
		return;
	}
	close(fd[1]); // Close writting end

	// Open socket as FILE stream since we need to print in lines anyway
	prog = fdopen(fd[0], "r");
	if (!prog)
		return;

	// Print line by line the output of the program
	while (fgets(line, LINELEN, prog)) {
		len = strlen(line);
		if (len > 2) { // Only print if line is not empty
			line[len] = '\0'; // Remove last newline char (\n) since we add it inside send_message()
			send_message(server, target, "%s", line); // The %s is needed to avoid interpeting format specifiers in output
		}
	}
	fclose(prog);
}

void print_cmd_output_unsafe(Irc server, const char *target, const char *cmd) {

	FILE *prog;
	char line[LINELEN];
	int len;

	// Open the program with arguments specified
	prog = popen(cmd, "r");
	if (!prog)
		return;

	while (fgets(line, LINELEN, prog)) {
		len = strlen(line);
		if (len > 2) {
			line[len] = '\0';
			send_message(server, target, "%s", line);
		}
	}
	pclose(prog);
}

STATIC size_t read_file(char **buf, const char *filename) {

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
	if (!st.st_size || st.st_size > CONFSIZE) {
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

void parse_config(yajl_val root, const char *config_file) {

	char errbuf[1024], *buf = NULL, *mpd_path, *mpd_random_file_path, *HOME;
	yajl_val val, array;
	int i;

	if (!read_file(&buf, config_file))
		exit_msg(config_file);

	root = yajl_tree_parse(buf, errbuf, sizeof(errbuf));
	if (!root)
		exit_msg("%s", errbuf);

	// Free original buffer since we have a duplicate in root now
	free(buf);

	val = yajl_tree_get(root, CFG("server"),       yajl_t_string);
	if (!val) exit_msg("server: missing / wrong type");
	cfg.server = YAJL_GET_STRING(val);

	val = yajl_tree_get(root, CFG("port"),         yajl_t_number);
	if (!val) exit_msg("port: missing / wrong type");
	cfg.port = YAJL_GET_NUMBER(val);

	val = yajl_tree_get(root, CFG("nick"),         yajl_t_string);
	if (!val) exit_msg("nick: missing / wrong type");
	cfg.nick = YAJL_GET_STRING(val);

	val = yajl_tree_get(root, CFG("user"),         yajl_t_string);
	if (!val) exit_msg("user: missing / wrong type");
	cfg.user = YAJL_GET_STRING(val);

	val = yajl_tree_get(root, CFG("nick_pwd"),     yajl_t_string);
	if (!val) exit_msg("nick_pwd: missing / wrong type");
	cfg.nick_pwd = YAJL_GET_STRING(val);

	val = yajl_tree_get(root, CFG("bot_version"),  yajl_t_string);
	if (!val) exit_msg("bot_version: missing / wrong type");
	cfg.bot_version = YAJL_GET_STRING(val);

	val = yajl_tree_get(root, CFG("github_repo"),  yajl_t_string);
	if (!val) exit_msg("github_repo: missing / wrong type");
	cfg.github_repo = YAJL_GET_STRING(val);

	val = yajl_tree_get(root, CFG("quit_message"), yajl_t_string);
	if (!val) exit_msg("quit_message: missing / wrong type");
	cfg.quit_msg = YAJL_GET_STRING(val);

	val = yajl_tree_get(root, CFG("mpd_database"), yajl_t_string);
	if (!val) exit_msg("mpd_database: missing / wrong type");
	cfg.mpd_database = YAJL_GET_STRING(val);

	val = yajl_tree_get(root, CFG("murmur_port"),  yajl_t_string);
	if (!val) exit_msg("murmur_port: missing / wrong type");
	cfg.murmur_port = YAJL_GET_STRING(val);

	val = yajl_tree_get(root, CFG("mpd_port"),     yajl_t_string);
	if (!val) exit_msg("mpd_port: missing / wrong type");
	cfg.mpd_port = YAJL_GET_STRING(val);

	val = yajl_tree_get(root, CFG("mpd_random_file"), yajl_t_string);
	if (!val) exit_msg("mpd_random_file: missing / wrong type");
	cfg.mpd_random_file = YAJL_GET_STRING(val);

	val = yajl_tree_get(root, CFG("oauth_consumer_key"),    yajl_t_string);
	if (!val) exit_msg("oauth_consumer_key: missing / wrong type");
	cfg.oauth_consumer_key = YAJL_GET_STRING(val);
	
	val = yajl_tree_get(root, CFG("oauth_consumer_secret"), yajl_t_string);
	if (!val) exit_msg("oauth_consumer_secret: missing / wrong type");
	cfg.oauth_consumer_secret = YAJL_GET_STRING(val);
	
	val = yajl_tree_get(root, CFG("oauth_token"),           yajl_t_string);
	if (!val) exit_msg("oauth_token: missing / wrong type");
	cfg.oauth_token = YAJL_GET_STRING(val);

	val = yajl_tree_get(root, CFG("oauth_token_secret"),    yajl_t_string);
	if (!val) exit_msg("oauth_token_secret: missing / wrong type");
	cfg.oauth_token_secret = YAJL_GET_STRING(val);
	
	// Expand tilde '~' by reading the HOME enviroment variable
	HOME = getenv("HOME");
	if (cfg.mpd_database[0] == '~') {
		mpd_path = malloc_w(PATHLEN);
		snprintf(mpd_path, PATHLEN, "%s%s", HOME, cfg.mpd_database + 1);
		cfg.mpd_database = mpd_path;
	}
	if (cfg.mpd_random_file[0] == '~') {
		mpd_random_file_path = malloc_w(PATHLEN);
		snprintf(mpd_random_file_path, PATHLEN, "%s%s", HOME, cfg.mpd_random_file + 1);
		cfg.mpd_random_file = mpd_random_file_path;
	}
	// Only accept true or false value
	val = yajl_tree_get(root, CFG("verbose"), yajl_t_any);
	if (!val) exit_msg("verbose: missing");

	if (val->type != yajl_t_true && val->type != yajl_t_false) exit_msg("verbose: wrong type");
 	cfg.verbose = YAJL_IS_TRUE(val);

	// Get the array of channels
	array = yajl_tree_get(root, CFG("channels"), yajl_t_array);
	if (!array) exit_msg("channels: missing / wrong type");
	cfg.channels_set = YAJL_GET_ARRAY(array)->len;

	if (cfg.channels_set > MAXCHANS) {
		cfg.channels_set = MAXCHANS;
		fprintf(stderr, "Channel limit (%d) reached. Ignoring rest\n", MAXCHANS);
	}
	for (i = 0; i < cfg.channels_set; i++) {
		val = YAJL_GET_ARRAY(array)->values[i];
		cfg.channels[i] = YAJL_GET_STRING(val);
	}
	// Get the array of quotes
	array = yajl_tree_get(root, CFG("fail_quotes"), yajl_t_array);
	if (!array) exit_msg("fail_quotes: missing / wrong type");
	cfg.quote_count = YAJL_GET_ARRAY(array)->len;

	if (cfg.quote_count > MAXQUOTES) {
		cfg.quote_count = MAXQUOTES;
		fprintf(stderr, "Quote limit (%d) reached. Ignoring rest\n", MAXQUOTES);
	}
	for (i = 0; i < cfg.quote_count; i++) {
		val = YAJL_GET_ARRAY(array)->values[i];
		cfg.quotes[i] = YAJL_GET_STRING(val);
	}

	// Get the array of twitter access list
	array = yajl_tree_get(root, CFG("twitter_access_list"), yajl_t_array);
	if (!array) exit_msg("twitter_access_list: missing / wrong type");
	cfg.access_list_count = YAJL_GET_ARRAY(array)->len;

	if (cfg.access_list_count > MAXLIST) {
		cfg.access_list_count = MAXLIST;
		fprintf(stderr, "Access list limit (%d) reached. Ignoring rest\n", MAXLIST);
	}
	for (i = 0; i < cfg.access_list_count; i++) {
		val = YAJL_GET_ARRAY(array)->values[i];
		cfg.twitter_access_list[i] = YAJL_GET_STRING(val);
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
