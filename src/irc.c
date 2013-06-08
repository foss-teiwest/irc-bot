#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <assert.h>
#include "socket.h"
#include "irc.h"
#include "gperf.h"
#include "helper.h"


struct channels {
	int channels_set;
	char channel[MAXCHANS][CHANLEN];
};

struct irc_type {
	int sock;
	char address[ADDRLEN];
	char port[PORTLEN];
	char nick[NICKLEN];
	char user[USERLEN];
	struct channels ch;
};

Irc connect_server(const char *address, const char *port) {

	Irc server = malloc_w(sizeof(struct irc_type));

	// Minimum validity checks
	if (strchr(address, '.') == NULL || atoi(port) > 65535)
		return NULL;

	strncpy(server->port, port, PORTLEN);
	strncpy(server->address, address, ADDRLEN);
	server->ch.channels_set = 0;

	server->sock = sock_connect(address, port);
	if (server->sock < 0)
		return NULL;

	return server;
}

char *set_nick(Irc server, const char *nick) {

	ssize_t n;
	char irc_msg[IRCLEN];

	assert(nick != NULL && "Error in set_nick");
	strncpy(server->nick, nick, NICKLEN);

	snprintf(irc_msg, IRCLEN, "NICK %s\r\n", server->nick);
	n = sock_write(server->sock, irc_msg, strlen(irc_msg));
	fputs(irc_msg, stdout);
	if (n < 0)
		exit_msg("Irc set nick failed");

	return server->nick;
}

char *set_user(Irc server, const char *user) {

	ssize_t n;
	char irc_msg[IRCLEN];

	assert(user != NULL && "Error in set_user");
	strncpy(server->user, user, USERLEN);

	snprintf(irc_msg, IRCLEN, "USER %s 0 * :%s\r\n", server->user, server->user);
	n = sock_write(server->sock, irc_msg, strlen(irc_msg));
	fputs(irc_msg, stdout);
	if (n < 0)
		exit_msg("Irc set user failed");

	return server->user;
}

void identify_nick(Irc server, char *pwd) {

	ssize_t n;
	char irc_msg[IRCLEN];

	snprintf(irc_msg, IRCLEN, "PRIVMSG nickserv :identify %s\r\n", pwd);
	n = sock_write(server->sock, irc_msg, strlen(irc_msg));
	if (n < 0)
		exit_msg("Irc identify nick failed");

	memset(pwd, 0, strlen(pwd)); // Zero-out password so it doesn't stay in memory
}

#ifdef TEST
	int join_channels(Irc server)
#else
	static int join_channels(Irc server)
#endif
{
	ssize_t n;
	char irc_msg[IRCLEN];
	int i;

	strcpy(irc_msg, "JOIN ");
	for (i = 0; i < server->ch.channels_set; i++) {
		strcat(irc_msg, server->ch.channel[i]);
		strcat(irc_msg, ",");
	}
	// Prepare message for sending
	i = strlen(irc_msg);
	irc_msg[i - 1] = '\r';
	irc_msg[i] = '\n';
	irc_msg[i + 1] = '\0';
	n = sock_write(server->sock, irc_msg, strlen(irc_msg));
	fputs(irc_msg, stdout);
	if (n < 0)
		exit_msg("Irc join channel failed");

	return server->ch.channels_set;
}

char *set_channel(Irc server, const char *channel) {

	assert(channel != NULL && "Error in set_channel");
	assert(channel[0] == '#' && "Missing # in channel");
	if (server->ch.channels_set == MAXCHANS) {
		fprintf(stderr, "Channel limit reached\n");
		return NULL;
	}
	strncpy(server->ch.channel[server->ch.channels_set], channel, CHANLEN);
	server->ch.channels_set++;

	return server->ch.channel[server->ch.channels_set];
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
	n = sock_write(server->sock, buf, strlen(buf));
	fputs(buf, stdout);
	if (n < 0)
		exit_msg("Irc ping reply failed");

	return buf;
}

char *parse_line(Irc server, char *line, Parsed_data pdata) {

	Function_list flist;
	int reply;

	if (line[0] == 'P') { // Check first line character
		ping_reply(server, line);
		return "PING";
	}
	pdata->sender = strtok(line + 1, " "); // Skip starting ':'
	if (pdata->sender == NULL)
		return NULL;
	pdata->command = strtok(NULL, " ");
	if (pdata->command == NULL)
		return NULL;
	pdata->message = strtok(NULL, "");
	if (pdata->message == NULL)
		return NULL;

	reply = atoi(pdata->command);
	if (reply == 0) {
		// Launch any actions registered to IRC commands
		flist = function_lookup(pdata->command, strlen(pdata->command));
		if (flist != NULL)
			flist->function(server, pdata);
	} else
		numeric_reply(server, reply);

	return pdata->command;
}

int numeric_reply(Irc server, int reply) {

	int i;

	switch (reply) {
		case NICKNAMEINUSE: // Change nick and resend the join command that got lost
			strcat(server->nick, "_");
			set_nick(server, server->nick);
			break;
		case ENDOFMOTD:
			i = join_channels(server);
			printf("%d channels joined\n", i);
			break;
	}
	return reply;
}

void irc_privmsg(Irc server, Parsed_data pdata) {

	Function_list flist;
	char *test_char;

	// Discard hostname from nickname
	test_char = strchr(pdata->sender, '!');
	if (test_char != NULL)
		*test_char = '\0';

	pdata->target = strtok(pdata->message, " ");
	if (pdata->target == NULL)
		return;
	if (strchr(pdata->target, '#') == NULL) // Not a channel message, reply on private
		pdata->target = pdata->sender;

	if (strcmp(pdata->sender, GITHUB_HOOK_NICK) == 0)
		pdata->command = "github_hook";
	else {
		// Bot commands must begin with '!'
		pdata->command = strtok(NULL, " ");
		if (pdata->command == NULL || *(pdata->command + 1) != '!')
			return;
		pdata->command += 2; // Skip starting ":!"
	}
	// Make sure bot command gets null terminated if there are no parameters
	// Even though we assign a read-only only string to pdata->message above in case of a nick match,
	// it won't enter the "if" body (there is a ':' char in the buffer still) so it won't crash trying to edit read-only memory
	pdata->message = strtok(NULL, "");
	if (pdata->message == NULL) {
		test_char = pdata->command;
		for (int i = 0; islower(*test_char) && i < USERLEN; test_char++, i++)
			; // Empty body
		*test_char = '\0';
	}
	// Find any actions registered to BOT commands
	flist = function_lookup(pdata->command, strlen(pdata->command));
	if (flist == NULL)
		return;

	switch (fork()) {
		case -1:
			flist->function(server, pdata); // Fork failed, run command in single process
			break;
		case 0:
			switch (fork()) {
				case -1: flist->function(server, pdata); // Fork failed, run command in 1st child
						_exit(EXIT_SUCCESS);
						break;
				case 0: // Run command in a new child and kill it's parent. That way we avoid zombies
					flist->function(server, pdata);
					_exit(EXIT_SUCCESS);
					break;
				default: // Kill 2nd's child parent
					_exit(EXIT_SUCCESS);
			}
			break;
		default: // Wait for the first child
			if (waitpid(-1, NULL, 0) < 0)
				perror("waitpid");
	}
}

void github_hook(Irc server, Parsed_data pdata) {

	int argc;
	char *temp, **argv, repo[CHANLEN + 1] = {0};

	argv = extract_params(pdata->message, &argc);
	if (argc < 4)
		goto cleanup;

	// Example github hook channel line: "[c-progs] none pushed 1 new commits to master: http://git.io/NoMeug"
	if (argv[0][1] != '[')
		goto cleanup;
	// Save repo name first without the surrounding '[]'
	strncpy(repo, argv[0] + 2, CHANLEN - 6);
	temp = strchr(repo, ']');
	if (temp == NULL)
		goto cleanup;
	*temp = ' ';
	*(temp + 1) = '\0';

	// Add the number of commits pushed
	strncat(repo, argv[3], 5);
	repo[strlen(repo)] = '\r'; // Needed for extract_params() to play nice
	pdata->message = repo;
	pdata->target = server->ch.channel[0]; // Sent to the first channel joined
	github(server, pdata);

cleanup:
	free(argv);
}

void send_message(Irc server, const char *target, const char *format, ...) {

	ssize_t n;
	va_list args;
	char msg[IRCLEN - CHANLEN], irc_msg[IRCLEN];

	va_start(args, format);
	vsnprintf(msg, IRCLEN - CHANLEN, format, args);
	snprintf(irc_msg, IRCLEN, "PRIVMSG %s :%s\r\n", target, msg);
	n = sock_write(server->sock, irc_msg, strlen(irc_msg));
	fputs(irc_msg, stdout);
	if (n < 0)
		exit_msg("Failed to send message");

	va_end(args);
}

void quit_server(Irc server, const char *msg) {

	ssize_t n;
	char irc_msg[IRCLEN];

	assert(msg != NULL && "Error in quit_server");

	snprintf(irc_msg, IRCLEN, "QUIT :%s\r\n", msg);
	n = sock_write(server->sock, irc_msg, strlen(irc_msg));
	fputs(irc_msg, stdout);
	if (n < 0)
		exit_msg("Quit failed");

	if (close(server->sock) < 0)
		perror("close");

	free(server);
}