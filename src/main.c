#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include "socket.h"
#include "irc.h"
#include "murmur.h"
#include "helper.h"

enum { IRC, MURM_LISTEN, MURM_ACCEPT };

int main(int argc, char *argv[]) {

	Irc freenode;
	struct pollfd pfd[3];
	int i, ready, murmur_fd;

	initialize(argc, argv);

	if ((murmur_fd = add_murmur_callbacks(MURMUR_PORT)) > 0) {
		close(murmur_fd);
		pfd[MURM_LISTEN].fd = murmur_fd = sock_listen("127.0.0.1", CB_LISTEN_PORT_S);
		pfd[MURM_LISTEN].events = POLLIN;
	}
	// Connect to server and set IRC details
	if ((freenode = connect_server(cfg.server, cfg.port)) == NULL)
		exit_msg("Irc connection failed");

	set_nick(freenode, cfg.nick);
	set_user(freenode, cfg.user);
	for (i = 0; i < cfg.channels_set; i++)
		join_channel(freenode, cfg.channels[i]);

	pfd[IRC].fd = get_socket(freenode);
	pfd[IRC].events = POLLIN;
	pfd[MURM_ACCEPT].fd = -1;
	pfd[MURM_ACCEPT].events = POLLIN;

	while ((ready = poll(pfd, 3, TIMEOUT)) > 0) {
		// Keep reading & parsing lines as long the connection is active and act on any registered actions found
		if (pfd[IRC].revents & POLLIN)
			while (parse_irc_line(freenode) > 0);
		if (pfd[MURM_LISTEN].revents & POLLIN) {
			pfd[MURM_ACCEPT].fd = sock_accept(murmur_fd);
			if (pfd[MURM_ACCEPT].fd > 0 && validate_murmur_connection(pfd[MURM_ACCEPT].fd) > 0)
				pfd[MURM_LISTEN].fd = -1; // Stop listening for connections
		}
		if (pfd[MURM_ACCEPT].revents & POLLIN) {
			if (listen_murmur_callbacks(freenode, pfd[MURM_ACCEPT].fd) <= 0) {
				close(pfd[MURM_ACCEPT].fd);
				pfd[MURM_ACCEPT].fd = -1;
				pfd[MURM_LISTEN].fd = murmur_fd; // Start listening again for Murmur connections
			}
		}
	}
	// If we reach here, it means we got disconnected from server. Exit with error (1)
	if (ready == 0)
		fprintf(stderr, "%d minutes passed without getting a message, exiting...\n", TIMEOUT / 1000 / 60);
	else if (ready == -1)
		perror("poll");

	quit_server(freenode, cfg.quit_msg);
	cleanup();
	return 1;
}
