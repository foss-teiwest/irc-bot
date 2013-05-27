#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include "socket.h"
#include "irc.h"
#include "gperf.h"
#include "helper.h"

struct irc_type {
	int sock;
	char address[SERVLEN];
	char port[PORTLEN];
	char nick[NICKLEN];
	char user[USERLEN];
	char channel[CHANLEN];
};

static const struct irc_type irc_servers[] = {
	[Freenode] = { .address = "chat.freenode.net", .port = "6667",
		.nick  = "foss_tesyd", .user = "bot", .channel = "foss-teimes" },
	[Grnet]    = { .address = "srv.irc.gr", .port = "6667",
		.nick  = "freestylerbot", .user = "bot", .channel = "randomblabla" },
	[Testing]  = { .address = "chat.freenode.net", .port = "6667",
		.nick  = "randombot", .user = "bot", .channel = "randombot" }
};


Irc connect_server(enum server_list sl) {

	Irc server = malloc_w(sizeof(struct irc_type));

	memcpy(server, &irc_servers[sl], sizeof(struct irc_type));
	server->sock = sock_connect(server->address, server->port);
	if (server->sock < 0)
		return NULL;

	return server;
}

char *set_nick(Irc server, const char *nick) {

	ssize_t n;
	char irc_msg[IRCLEN];

	if (nick != NULL)
		strncpy(server->nick, nick, NICKLEN);

	snprintf(irc_msg, IRCLEN, "NICK %s\r\n", server->nick);
	fputs(irc_msg, stdout);
	n = sock_write(server->sock, irc_msg, strlen(irc_msg));
	if (n < 0)
		exit_msg("Irc set nick failed");

	return server->nick;
}

char *set_user(Irc server, const char *user) {

	ssize_t n;
	char irc_msg[IRCLEN];

	if (user != NULL)
		strncpy(server->user, user, USERLEN);

	snprintf(irc_msg, IRCLEN, "USER %s 0 * :%s\r\n", server->user, server->user);
	fputs(irc_msg, stdout);
	n = sock_write(server->sock, irc_msg, strlen(irc_msg));
	if (n < 0)
		exit_msg("Irc set user failed");

	return server->user;
}

char *join_channel(Irc server, const char *channel) {

	ssize_t n;
	char irc_msg[IRCLEN];

	if (channel != NULL)
		strncpy(server->channel, channel, CHANLEN);

	snprintf(irc_msg, IRCLEN, "JOIN #%s\r\n", server->channel);
	fputs(irc_msg, stdout);
	n = sock_write(server->sock, irc_msg, strlen(irc_msg));
	if (n < 0)
		exit_msg("Irc join channel failed");

	return server->channel;
}

ssize_t get_line(Irc server, char *buf) {

	ssize_t n;

	n = sock_readline(server->sock, buf, IRCLEN);
	fputs(buf, stdout);

	return n;
}

char *ping_reply(Irc server, char *buf) {

	ssize_t n;

	buf[1] = 'O';
	fputs(buf, stdout);
	n = sock_write(server->sock, buf, strlen(buf));
	if (n < 0)
		exit_msg("Irc ping reply failed");

	return buf;
}

char *parse_line(Irc server, char *line, Parsed_data pdata) {

	Function_list flist;

	if (line[0] == 'P') { // Check first line character
		ping_reply(server, line);
		return "PING";
	}
	pdata->nick = strtok(line + 1, "!"); // Skip starting ':'
	if (pdata->nick == NULL)
		return NULL;
	if (strtok(NULL, " ") == NULL) // Ignore hostname
		return NULL;
	pdata->command = strtok(NULL, " ");
	if (pdata->command == NULL)
		return NULL;
	pdata->message = strtok(NULL, "");
	if (pdata->message == NULL)
		return NULL;

	// Launch any actions registered to IRC commands
	flist = function_lookup(pdata->command, strlen(pdata->command));
	if (flist != NULL)
		flist->function(server, pdata);

	return pdata->command;
}

void irc_privmsg(Irc server, Parsed_data pdata) {

	Function_list flist;
	char *command_char;
	pid_t pid;

	pdata->target = strtok(pdata->message, " ");
	if (pdata->target == NULL)
		return;
	if (strchr(pdata->target, '#') == NULL) // Not a channel message, reply on private
		pdata->target = pdata->nick;

	// Bot commands must begin with '@'
	pdata->command = strtok(NULL, " ");
	if (pdata->command == NULL || *(pdata->command + 1) != '@')
		return;
	pdata->command += 2; // Skip starting ":@"

	// Make sure bot command gets null terminated if there are no parameters
	pdata->message = strtok(NULL, "");
	if (pdata->message == NULL) {
		command_char = pdata->command;
		for (int i = 0; islower(*command_char) && i < USERLEN; command_char++, i++) ; // No body
		*command_char = '\0';
	}
	// Find any actions registered to BOT commands
	flist = function_lookup(pdata->command, strlen(pdata->command));
	if (flist == NULL)
		return;

	pid = fork();
	if (pid < 0)
		flist->function(server, pdata); // Fork failed, run command in single process
	else if (pid == 0) {
		pid = fork();
		if (pid == 0) { // Run command in a new child and kill it's parent. That way we avoid zombies
			flist->function(server, pdata);
			_exit(EXIT_SUCCESS);
		}
		else if (pid > 0) // Kill 2nd's child parent
			_exit(EXIT_SUCCESS);
	} else { // Wait for the first child
		pid = waitpid((pid_t) -1, NULL, 0);
		if (pid < 0)
			perror("waitpid");
	}
}

void send_message(Irc server, const char *target, const char *format, ...) {

	ssize_t n;
	va_list args;
	char msg[IRCLEN - CHANLEN], irc_msg[IRCLEN];

	va_start(args, format);
	vsnprintf(msg, IRCLEN - CHANLEN, format, args);
	snprintf(irc_msg, IRCLEN, "PRIVMSG %s :%s\r\n", target, msg);
	fputs(irc_msg, stdout);
	n = sock_write(server->sock, irc_msg, strlen(irc_msg));
	if (n < 0)
		exit_msg("Failed to send message");

	va_end(args);
}

void quit_server(Irc server, const char *msg) {

	ssize_t n;
	char irc_msg[IRCLEN];

	if (msg == NULL)
		msg = "";

	snprintf(irc_msg, IRCLEN, "QUIT :%s\r\n", msg);
	fputs(irc_msg, stdout);
	n = sock_write(server->sock, irc_msg, strlen(irc_msg));
	if (n < 0)
		exit_msg("Quit failed");

	if (close(server->sock) < 0)
		perror("close");

	free(server);
}