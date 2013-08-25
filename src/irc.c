#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <assert.h>
#include <signal.h>
#include <curl/curl.h>
#include <yajl/yajl_tree.h>
#include "socket.h"
#include "irc.h"
#include "gperf.h"
#include "helper.h"

// Wrapper functions. If VA_ARGS is NULL (last 2 args) then ':' will be ommited. Do not call _send_irc_command() directly
#define send_nick_command(server, target)    _send_irc_command(server, "NICK", target, NULL, NULL)
#define send_user_command(server, target)    _send_irc_command(server, "USER", target, NULL, NULL)
#define send_channel_command(server, target) _send_irc_command(server, "JOIN", target, NULL, NULL)
#define send_ping_command(server, target)    _send_irc_command(server, "PONG", target, NULL, NULL)
#define send_quit_command(server, target)    _send_irc_command(server, "QUIT", "", target, NULL)

struct irc_type {
	int sock;
	char address[ADDRLEN];
	char port[PORTLEN];
	char nick[NICKLEN];
	char user[USERLEN];
	struct {
		int channels_set;
		char channels[MAXCHANS][CHANLEN];
	} ch;
};

pid_t main_pid;
extern yajl_val yajl_root;

int get_socket(Irc server) {

	return server->sock;
}

Irc connect_server(const char *address, const char *port) {

	Irc server = malloc_w(sizeof(*server));

	// Minimum validity checks
	if (strchr(address, '.') == NULL || atoi(port) > 65535)
		return NULL;

	server->sock = sock_connect(address, port);
	if (server->sock < 0)
		return NULL;

	fcntl(server->sock, F_SETFL, O_NONBLOCK); // Set socket to non-blocking mode
	curl_global_init(CURL_GLOBAL_ALL); // Initialize curl library
	signal(SIGCHLD, SIG_IGN); // Make child processes not leave zombies behind when killed
	main_pid = getpid(); // store our process id to help exit_msg function exit appropriately

	strncpy(server->port, port, PORTLEN);
	strncpy(server->address, address, ADDRLEN);
	server->ch.channels_set = 0;

	return server;
}

void set_nick(Irc server, const char *nick) {

	assert(nick != NULL && "Error in set_nick");
	strncpy(server->nick, nick, NICKLEN);
	send_nick_command(server, server->nick);
}

void set_user(Irc server, const char *user) {

	char user_with_flags[USERLEN * 2 + 6];

	assert(user != NULL && "Error in set_user");
	strncpy(server->user, user, USERLEN);

	snprintf(user_with_flags, USERLEN * 2 + 6, "%s 0 * :%s", server->user, server->user);
	send_user_command(server, user_with_flags);
}

void set_channels(Irc server, char *channels[], int channels_set) {

	int i;

	for (i = 0; i < channels_set; i++) {
		assert(channels[i] != NULL   && "Error in set_channel");
		assert(channels[i][0] == '#' && "Missing # in channel");
		strncpy(server->ch.channels[server->ch.channels_set++], channels[i], CHANLEN);
	}
}

int join_channel(Irc server, const char *channel) {

	int i;

	if (channel == NULL) {
		for (i = 0; i < server->ch.channels_set; i++)
			send_channel_command(server, server->ch.channels[i]);
		return server->ch.channels_set;
	}
	send_channel_command(server, channel);

	return 1;
}

ssize_t parse_irc_line(Irc server) {

	char *test_char, line[IRCLEN + 1];
	Parsed_data pdata;
	Function_list flist;
	int reply;
	ssize_t n;

	// Read raw line from server. Example: ":laxanofido!~laxanofid@snf-23545.vm.okeanos.grnet.gr PRIVMSG #foss-teimes :How YA doing fossbot"
	n = sock_readline(server->sock, line, IRCLEN);
	if (n <= 0)
		return n;

	if (cfg.verbose)
		fputs(line, stdout);

	// Check for server ping request. Example: "PING :wolfe.freenode.net"
	// If we match PING then change the 2nd char to 'O' and terminate the argument before sending back
	if (strncmp(line, "PING", 4) == 0) {
		test_char = strrchr(line, '\r');
		*test_char = '\0';
		send_ping_command(server, line + 5);
		return n;
	}
	// Store the sender of the message / server command without the leading ':'.
	// Examples: "laxanofido!~laxanofid@snf-23545.vm.okeanos.grnet.gr", "wolfe.freenode.net"
	pdata.sender = strtok(line + 1, " ");
	if (pdata.sender == NULL)
		return n;

	// Store the server command. Examples: "PRIVMSG", "MODE", "433"
	pdata.command = strtok(NULL, " ");
	if (pdata.command == NULL)
		return n;

	// Store everything that comes after the server command
	// Examples: "#foss-teimes :How YA doing fossbot_", "fossbot :How YA doing fossbot"
	pdata.message = strtok(NULL, "");
	if (pdata.message == NULL)
		return n;

	// Initialize the last struct member to silence compiler warnings
	pdata.target = NULL;

	// Find out if server command is a numeric reply
	reply = atoi(pdata.command);
	if (reply == 0) {
		// Find & launch any functions registered to IRC commands
		flist = function_lookup(pdata.command, strlen(pdata.command));
		if (flist != NULL)
			flist->function(server, pdata);
	} else
		numeric_reply(server, reply);

	return n;
}

int numeric_reply(Irc server, int reply) {

	switch (reply) {
	case NICKNAMEINUSE: // Change our nick
		strcat(server->nick, "_");
		set_nick(server, server->nick);
		break;
	case ENDOFMOTD: // Join all channels registered with set_channel() after receiving MOTD
		join_channel(server, NULL);
		break;
	}
	return reply;
}

void irc_privmsg(Irc server, Parsed_data pdata) {

	Function_list flist;
	char *test_char;

	// Discard hostname from nickname. "laxanofido!~laxanofid@snf-23545.vm.okeanos.grnet.gr" becomes "laxanofido"
	test_char = strchr(pdata.sender, '!');
	if (test_char != NULL)
		*test_char = '\0';

	// Store message destination. Example channel: "#foss-teimes" or private: "fossbot"
	pdata.target = strtok(pdata.message, " ");
	if (pdata.target == NULL)
		return;

	// If target is not a channel, reply on private back to sender instead
	if (strchr(pdata.target, '#') == NULL)
		pdata.target = pdata.sender;

	// Example commands we might receive: ":!url in.gr", ":\x01VERSION\x01"
	pdata.command = strtok(NULL, " ");
	if (pdata.command == NULL)
		return;
	pdata.command++; // Skip leading ":" character

	// Make sure BOT command / CTCP request gets null terminated if there are no parameters
	pdata.message = strtok(NULL, "");
	if (pdata.message == NULL) {
		test_char = strrchr(pdata.command, '\r');
		*test_char = '\0';
	}
	// Bot commands must begin with '!'
	if (*pdata.command == '!') {
		pdata.command++; // Skip leading '!' before passing the command

		// Query our hash table for any functions registered to BOT commands
		flist = function_lookup(pdata.command, strlen(pdata.command));
		if (flist == NULL)
			return;

		// Launch the function in a new process
		switch (fork()) {
		case 0:
			flist->function(server, pdata);
			_exit(EXIT_SUCCESS);
		case -1:
			perror("fork");
		}
	}
	// CTCP requests must be received in private & begin with ascii char 1
	else if (*pdata.command == '\x01' && pdata.target == pdata.sender) {
		if (strncmp(pdata.command + 1, "VERSION", 7) == 0) // Skip the leading escape char
			send_notice(server, pdata.sender, "\x01%s\x01", cfg.bot_version);
	}
}

void irc_notice(Irc server, Parsed_data pdata) {

	bool temp;

	// notice destination
	pdata.target = strtok(pdata.message, " ");
	if (pdata.target == NULL)
		return;

	// Grab the message
	pdata.message = strtok(NULL, "");
	if (pdata.message == NULL)
		return;

	// Skip leading ':'
	pdata.message++;

	if (strncmp(pdata.message, "This nickname is registered", 27) == 0) {
		temp = cfg.verbose;
		cfg.verbose = false;
		send_message(server, "nickserv", "identify %s", cfg.nick_pwd);
		memset(cfg.nick_pwd, 0, strlen(cfg.nick_pwd));
		cfg.verbose = temp;
	}
}

void irc_kick(Irc server, Parsed_data pdata) {

	char *test_char, *victim;

	// Discard hostname from nickname
	test_char = strchr(pdata.sender, '!');
	if (test_char != NULL)
		*test_char = '\0';

	// Which channel did the kick happen
	pdata.target = strtok(pdata.message, " ");
	if (pdata.target == NULL)
		return;

	// Who got kicked
	victim = strtok(NULL, " ");
	if (victim == NULL)
		return;

	// Rejoin and send a message back to the one who kicked us
	// We check len + 1 (hit null char) to avoid matching extra chars after our nick
	if (strncmp(victim, server->nick, strlen(server->nick) + 1) == 0) {
		sleep(5);
		send_channel_command(server, pdata.target);
		send_message(server, pdata.target, "%s magkas...", pdata.sender);
	}
}

void _send_irc_command(Irc server, const char *type, const char *target, const char *format, ...) {

	va_list args;
	char msg[IRCLEN - CHANLEN], irc_msg[IRCLEN];

	va_start(args, format);
	vsnprintf(msg, IRCLEN - CHANLEN, format, args);
	if (*msg)
		snprintf(irc_msg, IRCLEN, "%s %s :%s\r\n", type, target, msg);
	else
		snprintf(irc_msg, IRCLEN, "%s %s\r\n", type, target);

	// Send message & print it on stdout
	if (sock_write(server->sock, irc_msg, strlen(irc_msg)) == -1)
		exit_msg("Failed to send message");

	if (cfg.verbose)
		fputs(irc_msg, stdout);

	va_end(args);
}

void quit_server(Irc server, const char *msg) {

	assert(msg != NULL && "Error in quit_server");
	send_quit_command(server, msg);

	if (close(server->sock) < 0)
		perror("close");

	curl_global_cleanup();
	yajl_tree_free(yajl_root);
	free(server);
}
