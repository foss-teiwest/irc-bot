#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/types.h>
#include "socket.h"
#include "irc.h"
#include "wrappers.h"

struct irc_type {
	int sock;
	char address[SERVLEN];
	char port[PORTLEN];
	char nick[NICKLEN];
	char user[USERLEN];
	char channel[CHANLEN];
};

static char buffer[BUFSIZE];
static char buffer2[BUFSIZE];

static const struct irc_type irc_servers[] = {
	[Freenode] = { .address = "chat.freenode.net", .port = "6667",
		.nick = "freestylerbot", .user = "bot", .channel = "randomblabla" },
	[Grnet] = {.address = "srv.irc.gr", .port = "6667",
		.nick = "freestylerbot", .user = "bot", .channel = "randomblabla" }
};

Irc select_server(int server_list) {

	Irc server;

	server = calloc_wrap(sizeof(struct irc_type));
	switch (server_list) {
		case Freenode:
			memcpy(server, &irc_servers[Freenode], sizeof(struct irc_type));
			break;
		case Grnet:
			memcpy(server, &irc_servers[Grnet], sizeof(struct irc_type));
			break;
	}
	return server;
}

int connect_server(Irc server) {

	int status = 0;

	server->sock = sock_connect(server->address, server->port);
	if (server->sock < 0)
		status = -1;

	set_nick(server, NULL);
	set_user(server, NULL);
	join_channel(server, NULL);

	return status;
}

void set_nick(Irc server, const char *nick) {

	ssize_t n;

	if (nick != NULL)
		strncpy(server->nick, nick, NICKLEN);

	snprintf(buffer, BUFSIZE, "NICK %s\r\n", server->nick);
	printf("<< %s", buffer);
	n = sock_write(server->sock, buffer, strlen(buffer));
	if (n < 0)
		exit_msg("Irc set nick failed");
}

void set_user(Irc server, const char *user) {

	ssize_t n;

	if (user != NULL)
		strncpy(server->user, user, USERLEN);

	snprintf(buffer, BUFSIZE, "USER %s 0 * :%s\r\n", server->user, server->user);
	printf("<< %s", buffer);
	n = sock_write(server->sock, buffer, strlen(buffer));
	if (n < 0)
		exit_msg("Irc set user failed");
}

void join_channel(Irc server, const char *channel) {

	ssize_t n;

	if (channel != NULL)
		strncpy(server->channel, channel, CHANLEN);

	snprintf(buffer, BUFSIZE, "JOIN #%s\r\n", server->channel);
	printf("<< %s", buffer);
	n = sock_write(server->sock, buffer, strlen(buffer));
	if (n < 0)
		exit_msg("Irc join channel failed");
}

ssize_t get_line(Irc server, char *buf) {

	ssize_t n;

	n = sock_readline(server->sock, buf, BUFSIZE);
	printf(">> %s", buf);

	return n;
}

void ping_reply(Irc server, char *buf) {

	ssize_t n;

	buf[1] = 'O';
	printf("<< %s", buf);
	n = sock_write(server->sock, buf, strlen(buf));
	if (n < 0)
		exit_msg("Irc ping reply failed");
}

void quit_server(Irc server) {

	close(server->sock);
	free(server);
}

char *get_nick(Irc server) {

	return server->nick;
}

bool parse_line(char *line, Parsed_data pdata) {

	char *token;

	pdata->nick = strtok(line + 1, "!"); // Skip ':' at the start
	if (pdata->nick == NULL)
		return false;
	token = strtok(NULL, " "); // Ignore hostname
	if (token == NULL)
		return false;
	pdata->command = strtok(NULL, " ");
	if (pdata->command == NULL)
		return false;
	pdata->target = strtok(NULL, " ");
	if (pdata->target == NULL)
		return false;
	pdata->message = strtok(NULL, "");
	if (pdata->message == NULL)
		return false;
	pdata->message++; //Skip ':' at the start

	return true;
}

void send_message(Irc server, const char *target, const char *format, ...) {

	ssize_t n;
	va_list args;

	va_start(args, format);
	vsnprintf(buffer, BUFSIZE - CHANLEN, format, args);
	snprintf(buffer2, BUFSIZE, "PRIVMSG %s :%s\r\n", target, buffer);
	printf("<< %s", buffer2);
	n = sock_write(server->sock, buffer2, strlen(buffer2));
	if (n < 0)
		exit_msg("Irc send message failed");

	va_end(args);
}