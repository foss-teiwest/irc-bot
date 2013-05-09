#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include "socket.h"
#include "irc.h"
#include "wrapper.h"

struct irc_type {
	int sock;
	bool conn_status;
	char nick[NICKLEN];
	char user[USERLEN];
	char curr_chans[MAXCHANS][CHANLEN];
};

static char buffer[BUFSIZE];

Irc irc_connect(int sock, const char *nick, const char *user) {
	Irc server;
	bool ret_value;

	server = malloc(sizeof(struct irc_type));
	if (server == NULL)
		exit_msg("Malloc: error in irc struct creation");

	memset(server, 0, sizeof(*server));
	server->sock = sock;

	ret_value = irc_nick(server, nick);
	if (ret_value == false) {
		irc_nick(server, "random23462");
		strncpy(server->nick, "random23462", NICKLEN);
	} else
		strncpy(server->nick, nick, NICKLEN);

	irc_user(server, user);
	strncpy(server->user, user, USERLEN);

	// irc_getline()
	// irc_parseline()

	// if (reply != 001)
	// 	return NULL;

	server->conn_status = true;
	return server;
}

bool irc_nick(Irc server, const char *nick) {

	ssize_t n;

	snprintf(buffer, BUFSIZE, "NICK %s\r\n", nick);
	n = sock_write(server->sock, buffer, strlen(buffer));
	if (n < 0)
		exit_msg("Irc set nick failed\n");

	return true;
}

void irc_user(Irc server, const char *user) {

	ssize_t n;

	snprintf(buffer, BUFSIZE, "USER %s 0 * :%s\r\n", user, user);
	n = sock_write(server->sock, buffer, strlen(buffer));
	if (n < 0)
		exit_msg("Irc set user failed\n");
}

void irc_join(Irc server, const char *channel) {

	ssize_t n;

	snprintf(buffer, BUFSIZE, "JOIN #%s\r\n", channel);
	n = sock_write(server->sock, buffer, strlen(buffer));
	if (n < 0)
		exit_msg("Irc join channel failed\n");
}

bool irc_connected(Irc server) {

	return server->conn_status;
}