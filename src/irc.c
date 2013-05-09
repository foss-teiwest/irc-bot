#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "irc.h"
#include "wrapper.h"

struct irc_type {
	int sock;
	bool conn_status;
	char nick[NICKLEN];
	char user[USERLEN];
	char curr_chans[MAXCHANNELS][CHANLEN];
};

Irc irc_connect(int sock, const char *nick, const char *user) {

	Irc server;
	int ret_value;

	server = malloc(sizeof(struct irc_type));
	if (server == NULL)
		exit_msg("Malloc: error in irc struct creation");

	memset(server, 0, sizeof(*server));
	server->sock = sock;

	ret_value = irc_nick(nick);
	if (ret_value < 0) {
		irc_nick("random23462");
		strncpy(server->nick, "random23462", NICKLEN);
	} else
		strncpy(server->nick, nick, NICKLEN);

	irc_user(user);
	strncpy(server->user, user, USERLEN);

	// irc_getline()
	// irc_parseline()

	// if (reply != 001)
	// 	return NULL;

	server->conn_status = 1;
	return server;
}