#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stddef.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <curl/curl.h>
#include <yajl/yajl_tree.h>
#include "irc.h"
#include "init.h"
#include "socket.h"
#include "queue.h"
#include "murmur.h"
#include "mpd.h"
#include "common.h"

// Reduce boilerplate code
#define CFG_GET(struct_name, root, field) struct_name.field = get_json_field(root, #field)

pid_t main_pid;
pthread_mutex_t *mtx;
struct mpd_info *mpd;
struct config_options cfg;

STATIC size_t read_file(char **buf, const char *filename) {

	FILE *file;
	struct stat st;
	size_t n = 0;

	file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "fopen error: ");
		return 0;
	}
	if (fstat(fileno(file), &st)) {
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

STATIC void parse_config(const char *config_file) {

	yajl_val root, val;
	char errbuf[1024], *buf = NULL, *mpd_path, *mpd_random_file_path, *HOME;

	if (!read_file(&buf, config_file))
		exit_msg("%s", config_file);

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
	CFG_GET(cfg, root, fifo_path);
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
	cfg.access_list_count = get_json_array(root, "access_list", cfg.access_list, MAXACCLIST);
}

void initialize(int argc, char *argv[]) {

	main_pid = getpid(); // store our process id to help exit_msg function exit appropriately
	signal(SIGCHLD, SIG_IGN); // Make child processes not leave zombies behind when killed
	signal(SIGPIPE, SIG_IGN); // Handle writing on closed sockets on our own
	curl_global_init(CURL_GLOBAL_ALL); // Initialize curl library
	setlinebuf(stdout); // Flush on each line

	// Accept config path as an optional argument
	parse_config(argc == 2 ? argv[1] : DEFAULT_CONFIG_NAME);

	if (*cfg.oauth_consumer_key && *cfg.oauth_consumer_secret && *cfg.oauth_token && *cfg.oauth_token_secret)
		cfg.twitter_details_set = true;

	// Set up a mutex that works across processes
	mtx = mmap_w(sizeof(pthread_mutex_t));
	pthread_mutexattr_t mtx_attr;
	pthread_mutexattr_init(&mtx_attr);
	pthread_mutexattr_setpshared(&mtx_attr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(mtx, &mtx_attr);
	pthread_mutexattr_destroy(&mtx_attr);
}

int setup_irc(Irc *server) {

	Mqueue mq;
	int ircfd;
	pthread_t tid;
	pthread_attr_t attr;

	*server = irc_connect(cfg.server, cfg.port);
	if (!*server)
		exit_msg("Irc connection failed");

	ircfd = get_socket(*server);
	mq = mqueue_init(ircfd);
	if (!mq)
		exit_msg("message queue initialization failed");

	set_mqueue(*server, mq);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (pthread_create(&tid, &attr, mqueue_start, mq))
		exit_msg("Could not start queue");

	pthread_attr_destroy(&attr);
	set_nick(*server, cfg.nick);
	set_user(*server, cfg.user);
	for (int i = 0; i < cfg.channels_set; i++)
		join_channel(*server, cfg.channels[i]);

	return ircfd;
}

int setup_mumble(void) {

	if (add_murmur_callbacks(cfg.murmur_port))
		return sock_listen(LOCALHOST, CB_LISTEN_PORT_S);

	fprintf(stderr, "Could not connect to Murmur\n");
	return -1;
}

int setup_mpd(void) {

	mpd = mmap_w(sizeof(*mpd));
	mpd->announce = OFF;
	if (!access(cfg.mpd_random_file, F_OK))
		mpd->random = ON;

	mpd->fd = mpd_connect(cfg.mpd_port);
	if (mpd->fd < 0)
		fprintf(stderr, "Could not connect to MPD\n");

	return mpd->fd;
}

int setup_fifo(FILE **stream) {

	mode_t old;
	struct stat st;
	int fifo, dummy;

	switch (stat(cfg.fifo_path, &st)) {
	case 0:
		if (S_ISFIFO(st.st_mode) && ((st.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO)) == FIFO_PERMISSIONS))
			break;

		if (remove(cfg.fifo_path))
			goto cleanup;

		// FALLTHROUGH
	default:
		// Ensure we get the permissions we asked
		old = umask(0);
		if (mkfifo(cfg.fifo_path, FIFO_PERMISSIONS))
			goto cleanup;

		umask(old); // Restore mask
	}
	fifo = open(cfg.fifo_path, O_RDONLY | O_NONBLOCK);
	if (fifo == -1)
		goto cleanup;

	// Ensure we never get EOF
	dummy = open(cfg.fifo_path, O_WRONLY);
	if (dummy == -1) {
		close(fifo);
		goto cleanup;
	}
	*stream = fdopen(fifo, "r");
	if (!*stream) {
		close(fifo);
		close(dummy);
		goto cleanup;
	}
	return fifo; // Success

cleanup:
	perror("Could not setup FIFO");
	return -1;
}

void cleanup(void) {

	pthread_mutex_destroy(mtx);
	munmap(mpd, sizeof(*mpd));
	munmap(mtx, sizeof(pthread_mutex_t));
	curl_global_cleanup();
}
