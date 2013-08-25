#include <stdio.h>
#include <poll.h>
#include "irc.h"
#include "helper.h"

int main(int argc, char *argv[]) {

	Irc freenode;
	struct pollfd pfd;
	ssize_t n;
	int ready;

	// Accept config path as an optional argument
	if (argc > 2) {
		fprintf(stderr, "Usage: %s [path_to_config]\n", argv[0]);
		return 1;
	} else if (argc == 2)
		parse_config(argv[1]);
	else
		parse_config("config.json");

	freenode = connect_server(cfg.server, cfg.port);
	if (freenode == NULL)
		exit_msg("Irc connection failed");

	set_nick(freenode, cfg.nick);
	set_user(freenode, cfg.user);
	set_channels(freenode, cfg.ch.channels, cfg.ch.channels_set);

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
	return 1;
}
