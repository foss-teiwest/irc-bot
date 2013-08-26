#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <curl/curl.h>
#include "helper.h"

pid_t main_pid;

int main(int argc, char *argv[]) {

	Irc freenode;
	yajl_val root = NULL;
	struct pollfd pfd;
	ssize_t n;
	int i, ready;

	// Accept config path as an optional argument
	if (argc > 2) {
		fprintf(stderr, "Usage: %s [path_to_config]\n", argv[0]);
		return 1;
	} else if (argc == 2)
		parse_config(root, argv[1]);
	else
		parse_config(root, "config.json");

	main_pid = getpid(); // store our process id to help exit_msg function exit appropriately
	signal(SIGCHLD, SIG_IGN); // Make child processes not leave zombies behind when killed
	signal(SIGPIPE, SIG_IGN); // Handle writing on closed sockets on our own
	curl_global_init(CURL_GLOBAL_ALL); // Initialize curl library

	// Connect to server and set IRC details
	freenode = connect_server(cfg.server, cfg.port);
	if (freenode == NULL)
		exit_msg("Irc connection failed");

	set_nick(freenode, cfg.nick);
	set_user(freenode, cfg.user);
	for (i = 0; i < cfg.ch.channels_set; i++)
		join_channel(freenode, cfg.ch.channels[i]);

	pfd.fd = get_socket(freenode);
	pfd.events = POLLIN;

	while ((ready = poll(&pfd, 1, TIMEOUT)) > 0) {
		// Keep reading & parsing lines as long the connection is active and act on any registered actions found
		if (pfd.revents & POLLIN) {
			while ((n = parse_irc_line(freenode)) > 0);
			if (n != -2)
				goto cleanup;
		}
	}
	// If we reach here, it means we got disconnected from server. Exit with error (1)
	if (ready == 0) {
		fprintf(stderr, "%d seconds passed without getting a message, exiting...\n", TIMEOUT);
		goto cleanup;
	}
	if (ready == -1)
		perror("poll");

cleanup:
	quit_server(freenode, cfg.quit_msg);
	yajl_tree_free(root);
	curl_global_cleanup();
	return 1;
}
