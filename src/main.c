#include <stdio.h>
#include <poll.h>
#include "socket.h"
#include "irc.h"
#include "murmur.h"
#include "mpd.h"
#include "common.h"

enum { IRC, MURM_LISTEN, MURM_ACCEPT, MPD };

int mpdfd;

int main(int argc, char *argv[]) {

	Irc freenode;
	struct pollfd pfd[4];
	int i, ready, murm_listenfd = -1;

	initialize(argc, argv);

	for (i = 0; i < SIZE(pfd); i++) {
		pfd[i].fd = -1;
		pfd[i].events = POLLIN;
	}
	if (add_murmur_callbacks(cfg.murmur_port))
		pfd[MURM_LISTEN].fd = murm_listenfd = sock_listen(LOCALHOST, CB_LISTEN_PORT_S);
	else
		fprintf(stderr, "Could not connect to Murmur\n");

	if ((pfd[MPD].fd = mpdfd = mpd_connect(cfg.mpd_port)) < 0)
		fprintf(stderr, "Could not connect to MPD\n");

	// Connect to server and set IRC details
	if (!(freenode = irc_connect(cfg.server, cfg.port)))
		exit_msg("Irc connection failed");

	pfd[IRC].fd = get_socket(freenode);
	set_nick(freenode, cfg.nick);
	set_user(freenode, cfg.user);
	for (i = 0; i < cfg.channels_set; i++)
		join_channel(freenode, cfg.channels[i]);

	while ((ready = poll(pfd, SIZE(pfd), TIMEOUT)) > 0) {
		// Keep reading & parsing lines as long the connection is active and act on any registered actions found
		if (pfd[IRC].revents & POLLIN)
			while (parse_irc_line(freenode) > 0);

		if (pfd[MURM_LISTEN].revents & POLLIN)
			if ((pfd[MURM_ACCEPT].fd = accept_murmur_connection(murm_listenfd)) > 0)
				pfd[MURM_LISTEN].fd = -1; // Stop listening for connections

		if (pfd[MURM_ACCEPT].revents & POLLIN) {
			if (!listen_murmur_callbacks(freenode, pfd[MURM_ACCEPT].fd)) {
				pfd[MURM_ACCEPT].fd = -1;
				pfd[MURM_LISTEN].fd = murm_listenfd; // Start listening again for Murmur connections
			}
		}
		if (pfd[MPD].revents & POLLIN)
			if (!print_song(freenode, default_channel(freenode)))
				pfd[MPD].fd = mpdfd = mpd_connect(cfg.mpd_port);
	}
	// If we reach here, it means we got disconnected from server. Exit with error (1)
	if (ready == -1)
		perror("poll");
	else
		fprintf(stderr, "%d minutes passed without getting a message, exiting...\n", TIMEOUT / 1000 / 60);

	quit_server(freenode, cfg.quit_msg);
	cleanup();
	return 1;
}
