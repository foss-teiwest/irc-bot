#include <stdio.h>
#include <poll.h>
#include "init.h"
#include "irc.h"
#include "murmur.h"
#include "mpd.h"
#include "common.h"

enum poll_array {IRC, MURM_LISTEN, MURM_ACCEPT, MPD, FIFO, TOTAL};

int main(int argc, char *argv[]) {

	Irc server;
	FILE *fifo;
	int ready;
	struct pollfd pfd[TOTAL];

	for (int i = 0; i < TOTAL; i++) {
		pfd[i].fd = -1;
		pfd[i].events = POLLIN;
	}
	initialize(argc, argv);
	pfd[IRC].fd         = setup_irc(&server);
	pfd[MURM_LISTEN].fd = setup_mumble();
	pfd[MPD].fd         = setup_mpd();
	pfd[FIFO].fd        = setup_fifo(&fifo);

	while ((ready = poll(pfd, TOTAL, POLL_TIMEOUT)) > 0) {
		// Read & parse all available lines and act on any registered actions found
		if (pfd[IRC].revents & POLLIN)
			while (parse_irc_line(server) > 0);

		if (pfd[MURM_LISTEN].revents & POLLIN) {
			pfd[MURM_ACCEPT].fd = accept_murmur_connection(pfd[MURM_LISTEN].fd);
			if (pfd[MURM_ACCEPT].fd > 0)
				pfd[MURM_LISTEN].events = 0; // Stop listening for connections
		}
		if (pfd[MURM_ACCEPT].revents & POLLIN) {
			if (!listen_murmur_callbacks(server, pfd[MURM_ACCEPT].fd)) {
				pfd[MURM_ACCEPT].fd = -1;
				pfd[MURM_LISTEN].events = POLLIN; // Start listening again for Murmur connections
			}
		}
		if (pfd[MPD].revents & POLLIN)
			if (!print_song(server, default_channel(server)))
				pfd[MPD].fd = mpd_connect(cfg.mpd_port);

		if (pfd[FIFO].revents & POLLIN)
			send_all_lines(server, default_channel(server), fifo);
	}
	// If we reach here, it means we got disconnected from server. Exit with error (1)
	if (ready == -1)
		perror("poll");
	else
		fprintf(stderr, "%d minutes passed without getting a message, exiting...\n", POLL_TIMEOUT / MILLISECS / 60);

	quit_server(server, cfg.quit_message);
	fclose(fifo);
	cleanup();
	return 1;
}
