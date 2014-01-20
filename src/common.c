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
		parse_config(root, DEFAULT_CONFIG_NAME);

	cfg.twitter_details_set = false;
	if (*cfg.oauth_consumer_key && *cfg.oauth_consumer_secret && *cfg.oauth_token && *cfg.oauth_token_secret)
		cfg.twitter_details_set = true;

	signal(SIGCHLD, SIG_IGN); // Make child processes not leave zombies behind when killed
	signal(SIGPIPE, SIG_IGN); // Handle writing on closed sockets on our own
	curl_global_init(CURL_GLOBAL_ALL); // Initialize curl library

	mpd_status = mmap(NULL, sizeof(*mpd_status), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (mpd_status == MAP_FAILED)
		perror("mmap");

	mpd_status->announce = OFF;
	mpd_status->random = access(cfg.mpd_random_file, F_OK) ? OFF : ON;
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

	int size, argc = 0;
	char *temp;

	// Make sure we have at least 1 parameter before proceeding
	if (!msg)
		return 0;

	// Null terminate the whole parameters line
	temp = strrchr(msg, '\r');
	if (!temp)
		return 0;

	*temp = '\0';

	// Allocate enough starting space for most bot commands
	*argv = malloc_w(STARTSIZE * sizeof(char *));
	size = STARTSIZE;

	// split parameters seperated by space or tab
	(*argv)[argc] = strtok(msg, " \t");
	while ((*argv)[argc]) {
		if (argc == size - 1) { // Double the array if it gets full
			*argv = realloc_w(*argv, size * 2 * sizeof(char *));
			size *= 2;
		}
		(*argv)[++argc] = strtok(NULL, " \t");
	}

	if (!argc)
		free(*argv);

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

STATIC char *get_json_field(yajl_val root, const char *field_name) {
	
	yajl_val val = yajl_tree_get(root, CFG(field_name), yajl_t_string);
	if (!val)
		exit_msg("%s: missing / wrong type", field_name);

	return YAJL_GET_STRING(val);
}

STATIC int get_json_array(yajl_val root, const char *array_name, char **array_to_fill, int max_entries) {

	yajl_val val, array;
	int i, array_size;

	array = yajl_tree_get(root, CFG(array_name), yajl_t_array);
	if (!array)
		exit_msg("%s: missing / wrong type", array_name);

	array_size = YAJL_GET_ARRAY(array)->len;
	if (array_size > max_entries) {
		array_size = max_entries;
		fprintf(stderr, "%s limit (%d) reached. Ignoring rest\n", array_name,  max_entries);
	}
	for (i = 0; i < array_size; i++) {
		val = YAJL_GET_ARRAY(array)->values[i];
		array_to_fill[i] = YAJL_GET_STRING(val);
	}
	
	return array_size;
}

void parse_config(yajl_val root, const char *config_file) {

	yajl_val val;
	char errbuf[1024], *buf = NULL, *mpd_path, *mpd_random_file_path, *HOME;

	if (!read_file(&buf, config_file))
		exit_msg(config_file);

	root = yajl_tree_parse(buf, errbuf, sizeof(errbuf));
	if (!root)
		exit_msg("%s", errbuf);

	// Free original buffer since we have a duplicate in root now
	free(buf);

	CFG_GET(cfg, root, server);	
	CFG_GET(cfg, root, port);
	CFG_GET(cfg, root, nick);
	CFG_GET(cfg, root, user);
	CFG_GET(cfg, root, nick_password);
	CFG_GET(cfg, root, bot_version);
	CFG_GET(cfg, root, quit_message);
	CFG_GET(cfg, root, github_repo);
	CFG_GET(cfg, root, murmur_port);
	CFG_GET(cfg, root, mpd_port);
	CFG_GET(cfg, root, mpd_database);
	CFG_GET(cfg, root, mpd_random_file);
	CFG_GET(cfg, root, twitter_profile_url);
	CFG_GET(cfg, root, oauth_consumer_key);
	CFG_GET(cfg, root, oauth_consumer_secret);
	CFG_GET(cfg, root, oauth_token);
	CFG_GET(cfg, root, oauth_token_secret);
	
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
	if (!val)
		exit_msg("verbose: missing");

	if (val->type != yajl_t_true && val->type != yajl_t_false)
		exit_msg("verbose: wrong type");

	cfg.verbose = YAJL_IS_TRUE(val);

	// Fill arrays
	cfg.channels_set = get_json_array(root, "channels", cfg.channels, MAXCHANS);
	cfg.quote_count = get_json_array(root, "fail_quotes", cfg.quotes, MAXQUOTES);
	cfg.access_list_count = get_json_array(root, "twitter_access_list", cfg.twitter_access_list, MAXLIST);
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
