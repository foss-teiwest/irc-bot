#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include "socket.h"
#include "irc.h"
#include "wrapper.h"

struct irc_type {
	int sock;
	char server[SERVLEN];
	char port[PORTLEN];
	bool is_connected;
	char nick[NICKLEN];
	char user[USERLEN];
	char channel[CHANLEN];
};

static char buffer[BUFSIZE];

Irc irc_select_server(enum Server_list server_list) {

	Irc server = malloc(sizeof(struct irc_type));
	if (server == NULL)
		exit_msg("Malloc: error in irc struct creation");
	memset(server, 0, sizeof(*server));

	switch (server_list) {
		case Freenode:
			strncpy(server->server, "chat.freenode.net", SERVLEN);
			strncpy(server->port, "6667", PORTLEN);
			strncpy(server->nick, "botten_anna", NICKLEN);
			strncpy(server->user, "bot", USERLEN);
			strncpy(server->channel, "randomblabla", CHANLEN);
			break;
	}
	return server;
}

int irc_connect_server(Irc server) {

	int ret_value, status = -2;

	server->sock = sock_connect(server->server, server->port);
	if (server->sock < 0)
		status = -1;

	ret_value = irc_set_nick(server, NULL);
	if (ret_value < 0)
		irc_set_nick(server, "random23462");

	irc_set_user(server, NULL);
	irc_join_channel(server, NULL);

	int reply = 001; // incomplete
	if (reply == 001) {
		server->is_connected = true;
		status = 0;
	}
	return status;
}

int irc_set_nick(Irc server, const char *nick) {

	ssize_t n;

	if (nick != NULL)
		strncpy(server->nick, nick, NICKLEN);

	snprintf(buffer, BUFSIZE, "NICK %s\r\n", server->nick);
	n = sock_write(server->sock, buffer, strlen(buffer));
	if (n < 0)
		exit_msg("Irc set nick failed\n");

	return 0;
}

void irc_set_user(Irc server, const char *user) {

	ssize_t n;

	if (user != NULL)
		strncpy(server->user, user, USERLEN);

	snprintf(buffer, BUFSIZE, "USER %s 0 * :%s\r\n", server->user, server->user);
	n = sock_write(server->sock, buffer, strlen(buffer));
	if (n < 0)
		exit_msg("Irc set user failed\n");
}

void irc_join_channel(Irc server, const char *channel) {

	ssize_t n;

	if (channel != NULL)
		strncpy(server->channel, channel, CHANLEN);

	snprintf(buffer, BUFSIZE, "JOIN #%s\r\n", server->channel);
	n = sock_write(server->sock, buffer, strlen(buffer));
	if (n < 0)
		exit_msg("Irc join channel failed\n");
}

bool irc_is_connected(Irc server) {

	return server->is_connected;
}

void irc_quit_server(Irc server) {

	close(server->sock);
	free(server);
}