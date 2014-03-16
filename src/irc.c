#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>
#include "socket.h"
#include "irc.h"
#include "gperf.h"
#include "common.h"

// Wrapper function. Since last 2 args (VA_ARGS) are NULL, ':' will be ommited. Do not call _irc_command() directly
#define irc_command(server, type, arg)  _irc_command(server, type, arg, NULL, (char *) NULL)

struct irc_type {
	int sock;
	int pipe[2];
	char line[IRCLEN + 1];
	size_t line_offset;
	char address[ADDRLEN];
	char port[PORTLEN];
	char nick[NICKLEN];
	char user[USERLEN];
	char channels[MAXCHANS][CHANLEN];
	int channels_set;
	bool isConnected;
};

extern pthread_mutex_t *mtx;
extern pid_t main_pid;

Irc irc_connect(const char *address, const char *port) {

	Irc server = CALLOC_W(sizeof(*server));

	// Minimum validity checks
	if (!strchr(address, '.'))
		return NULL;

	server->sock = sock_connect(address, port);
	if (server->sock < 0)
		return NULL;

	if (pipe(server->pipe) < 0) {
		perror("pipe");
		return NULL;
	}
	fcntl(server->sock, F_SETFL, O_NONBLOCK); // Set socket to non-blocking mode
	strncpy(server->address, address, ADDRLEN);
	strncpy(server->port, port, PORTLEN);

	return server;
}

int get_socket(Irc server) {

	return server->sock;
}

char *default_channel(Irc server) {

	return server->channels[0];
}

STATIC bool user_in_access_list(const char *nick) {

	int i;

	for (i = 0; i < cfg.access_list_count; i++)
		if (streq(nick, cfg.access_list[i]))
			break;

	return i != cfg.access_list_count;
}

STATIC bool user_is_identified(Irc server, const char *nick) {

	int auth_level = 0;

	pthread_mutex_lock(mtx);
	send_message(server, "NickServ", "ACC %s", nick);
	if (read(server->pipe[0], &auth_level, 4) != 4)
		perror(__func__);

	close(server->pipe[0]);
	close(server->pipe[1]);
	pthread_mutex_unlock(mtx);

	return auth_level == 3;
}

bool user_has_access(Irc server, const char *nick) {

	// Avoid deadlock
	assert(getpid() != main_pid);

	if (user_in_access_list(nick) && user_is_identified(server, nick))
		return true;

	return false;
}

void set_nick(Irc server, const char *nick) {

	assert(nick);
	strncpy(server->nick, nick, NICKLEN);
	irc_command(server, "NICK", server->nick);
}

void set_user(Irc server, const char *user) {

	char user_with_flags[USERLEN * 2 + 6];

	assert(user);
	strncpy(server->user, user, USERLEN);

	snprintf(user_with_flags, USERLEN * 2 + 6, "%s 0 * :%s", server->user, server->user);
	irc_command(server, "USER", user_with_flags);
}

int join_channel(Irc server, const char *channel) {

	int i;

	if (channel) {
		assert(channel[0] == '#');
		if (server->channels_set == MAXCHANS) {
			fprintf(stderr, "Channel limit reached (%d)\n", MAXCHANS);
			return 0;
		}
		strncpy(server->channels[server->channels_set++], channel, CHANLEN);
		if (server->isConnected)
			irc_command(server, "JOIN", server->channels[server->channels_set - 1]);

		return 1;
	}

	i = 0;
	if (server->isConnected)
		while (i < server->channels_set)
			irc_command(server, "JOIN", server->channels[i++]);

	return i;
}

ssize_t parse_irc_line(Irc server) {

	Parsed_data pdata;
	Command command;
	int reply;
	ssize_t n;

	// Read raw line from server. Example: ":laxanofido!~laxanofid@snf-23545.vm.okeanos.grnet.gr PRIVMSG #foss-teimes :How YA doing fossbot"
	n = sock_readline(server->sock, server->line + server->line_offset, IRCLEN - server->line_offset);
	if (n <= 0) {
		if (n != -EAGAIN)
			exit_msg("IRC connection closed\n");

		server->line_offset = strlen(server->line);
		return n;
	}
	// Clear offset and remove IRC protocol terminators \r\n
	n += server->line_offset - 2;
	server->line[n] = '\0';
	server->line_offset = 0;

	if (cfg.verbose)
		puts(server->line);

	// Check for server ping request. Example: "PING :wolfe.freenode.net"
	if (starts_with(server->line, "PING")) {
		irc_command(server, "PONG", server->line + 5);
		return n;
	}

	// Store the sender of the message / server command without the leading ':'.
	// Examples: "laxanofido!~laxanofid@snf-23545.vm.okeanos.grnet.gr", "wolfe.freenode.net"
	pdata.sender = strtok(server->line + 1, " ");
	if (!pdata.sender)
		return n;

	// Store the server command. Examples: "PRIVMSG", "MODE", "433"
	pdata.command = strtok(NULL, " ");
	if (!pdata.command)
		return n;

	// Store everything that comes after the server command
	// Examples: "#foss-teimes :How YA doing fossbot_", "fossbot :How YA doing fossbot"
	pdata.message = strtok(NULL, "");
	if (!pdata.message)
		return n;

	// Initialize the last struct member to silence compiler warnings
	pdata.target = NULL;

	// Find out if server command is a numeric reply
	reply = atoi(pdata.command);
	if (reply)
		numeric_reply(server, reply);
	else {
		// Find & launch any functions registered to IRC commands
		command = command_lookup(pdata.command, strlen(pdata.command));
		if (command)
			command->function(server, pdata);
	}

	return n;
}

int numeric_reply(Irc server, int reply) {

	switch (reply) {
	case NICKNAMEINUSE: // Change our nick
		strcat(server->nick, "_");
		set_nick(server, server->nick);
		break;
	case ENDOFMOTD: // Join all channels set before
		server->isConnected = true;
		join_channel(server, NULL);
		break;
	}

	return reply;
}

void irc_privmsg(Irc server, Parsed_data pdata) {

	Command command;

	// Discard hostname from nickname. "laxanofido!~laxanofid@snf-23545.vm.okeanos.grnet.gr" becomes "laxanofido"
	if (!null_terminate(pdata.sender, '!'))
		return;

	// Store message destination. Example channel: "#foss-teimes" or private: "fossbot"
	pdata.target = strtok(pdata.message, " ");
	if (!pdata.target)
		return;

	// If target is not a channel, reply on private back to sender instead
	if (!strchr(pdata.target, '#'))
		pdata.target = pdata.sender;

	// Example commands we might receive: ":!url in.gr", ":\x01VERSION\x01"
	pdata.command = strtok(NULL, " ");
	if (!pdata.command)
		return;

	pdata.command++; // Skip leading ":" character

	// Grab the rest arguments if any or set null
	pdata.message = strtok(NULL, "");

	// Bot commands must begin with '!'
	if (*pdata.command == '!') {
		pdata.command++; // Skip leading '!' before passing the command

		// Query our hash table for any functions registered to BOT commands
		command = command_lookup(pdata.command, strlen(pdata.command));
		if (!command)
			return;

		// Launch the function in a new process
		switch (fork()) {
		case 0:
			command->function(server, pdata);
			_exit(EXIT_SUCCESS);
		case -1:
			perror("fork");
		}
	}
	// CTCP requests must begin with ascii char 1
	else if (*pdata.command == '\x01') {
		if (starts_with(pdata.command + 1, "VERSION")) // Skip the leading escape char
			send_notice(server, pdata.sender, "\x01VERSION %s\x01", cfg.bot_version);
	}
}

void irc_notice(Irc server, Parsed_data pdata) {

	char *test;
	bool temp;
	int auth_level;

	// Discard hostname from nickname
	if (!null_terminate(pdata.sender, '!'))
		return;

	// Notice destination
	pdata.target = strtok(pdata.message, " ");
	if (!pdata.target)
		return;

	// Grab the message
	pdata.message = strtok(NULL, "");
	if (!pdata.message)
		return;

	// Skip leading ':'
	pdata.message++;

	if (!streq(pdata.sender, "NickServ"))
		return;

	test = strstr(pdata.message, "ACC");
	if (test) {
		auth_level = atoi(test + 4);
		if (write(server->pipe[1], &auth_level, 4) != 4)
			perror(__func__);
	} else if (starts_with(pdata.message, "This nickname is registered")) {
		temp = cfg.verbose;
		cfg.verbose = false;
		send_message(server, pdata.sender, "identify %s", cfg.nick_password);
		memset(cfg.nick_password, '\0', strlen(cfg.nick_password));
		cfg.verbose = temp;
	}
}

void irc_kick(Irc server, Parsed_data pdata) {

	int i;
	char *victim;

	// Discard hostname from nickname
	if (!null_terminate(pdata.sender, '!'))
		return;

	// Which channel did the kick happen
	pdata.target = strtok(pdata.message, " ");
	if (!pdata.target)
		return;

	// Who got kicked
	victim = strtok(NULL, " ");
	if (!victim)
		return;

	// Rejoin and send a message back to the one who kicked us
	if (streq(victim, server->nick)) {
		sleep(3);

		// Find the channel we got kicked on and remove it from our list
		// TODO verify if we actually rejoined the channel
		for (i = 0; i < server->channels_set; i++)
			if (streq(pdata.target, server->channels[i]))
				break;

		strncpy(server->channels[i], server->channels[--server->channels_set], CHANLEN);
		join_channel(server, pdata.target);
		send_message(server, pdata.target, "magkas %s...", pdata.sender);
	}
}

void _irc_command(Irc server, const char *type, const char *target, const char *format, ...) {

	va_list args;
	char msg[IRCLEN - 50], irc_msg[IRCLEN];

	if (format) {
		va_start(args, format);
		vsnprintf(msg, sizeof(msg), format, args);
		snprintf(irc_msg, IRCLEN, "%s %s :%s\r\n", type, target, msg);
		va_end(args);
	} else
		snprintf(irc_msg, IRCLEN, "%s %s\r\n", type, target);

	// Send message & print it on stdout
	if (sock_write_non_blocking(server->sock, irc_msg, strlen(irc_msg)) == -1)
		exit_msg("Failed to send message\n");

	if (cfg.verbose)
		fputs(irc_msg, stdout);
}

void quit_server(Irc server, const char *msg) {

	char exit_msg[IRCLEN - 200] = ":";

	assert(msg);
	strncat(exit_msg, msg, sizeof(exit_msg) - 1);
	irc_command(server, "QUIT", exit_msg);

	if (close(server->sock) < 0)
		perror(__func__);

	free(server);
}
