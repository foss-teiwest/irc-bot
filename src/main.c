#include <stdio.h>
#include <poll.h>
#include "init.h"
#include "irc.h"
#include "murmur.h"
#include "mpd.h"
#include "common.h"

int main(int argc, char *argv[]) {

	Irc irc_server;
	FILE *fifo;
	int i, ready;
	struct pollfd pfd[TOTAL];

	for (i = 0; i < TOTAL; i++) {
		pfd[i].fd = -1;
		pfd[i].events = POLLIN;
	}
	initialize(argc, argv);
	irc_server          = setup_irc();
	pfd[IRC].fd         = get_socket(irc_server);
	pfd[MURM_LISTEN].fd = setup_mumble();
	pfd[MPD].fd         = setup_mpd();
	pfd[FIFO].fd        = setup_fifo(&fifo);

	while ((ready = poll(pfd, TOTAL, POLL_TIMEOUT)) > 0) {
		// Read & parse all available lines and act on any registered actions found
		if (pfd[IRC].revents & POLLIN)
			while (parse_irc_line(irc_server) > 0);

		if (pfd[MURM_LISTEN].revents & POLLIN) {
			pfd[MURM_ACCEPT].fd = accept_murmur_connection(pfd[MURM_LISTEN].fd);
			if (pfd[MURM_ACCEPT].fd > 0)
				pfd[MURM_LISTEN].events = 0; // Stop listening for connections
		}
		if (pfd[MURM_ACCEPT].revents & POLLIN) {
			if (!listen_murmur_callbacks(irc_server, pfd[MURM_ACCEPT].fd)) {
				pfd[MURM_ACCEPT].fd = -1;
				pfd[MURM_LISTEN].events = POLLIN; // Start listening again for Murmur connections
			}
		}
		if (pfd[MPD].revents & POLLIN)
			if (!print_song(irc_server, default_channel(irc_server)))
				pfd[MPD].fd = mpd_connect(cfg.mpd_port);

		if (pfd[FIFO].revents & POLLIN)
			send_all_lines(irc_server, default_channel(irc_server), fifo);
	}
	// If we reach here, it means we got disconnected from server. Exit with error (1)
	if (ready == -1)
		perror("poll");
	else
		fprintf(stderr, "%d minutes passed without getting a message, exiting...\n", POLL_TIMEOUT / MILLISECS / 60);

	quit_server(irc_server, cfg.quit_message);
	fclose(fifo);
	cleanup();
	return 1;
}
