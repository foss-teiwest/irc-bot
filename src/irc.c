#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>
#include "socket.h"
#include "irc.h"
#include "gperf.h"
#include "common.h"
#include "init.h"
#include "database.h"
#include "queue.h"

struct irc_type {
	int conn;
	Mqueue mqueue;
	pthread_mutex_t *mtx;
	int pipe[RDWR];
	char line[IRCLEN + 1];
	size_t line_offset;
	char address[ADDRLEN + 1];
	char port[PORTLEN + 1];
	char nick[NICKLEN + 1];
	char user[USERLEN + 1];
	char channels[MAXCHANS][CHANLEN + 1];
	int channels_set;
	bool connected;
};

struct command_info {
	Irc server;
	Command *cmd;
	char *line;
	struct parsed_data pdata;
};

#define ctcp_reply(server, target, format, ...) _irc_command(server, "NOTICE",  target, "\x01" format "\x01", __VA_ARGS__)

STATIC void pre_launch_command(Irc server, struct parsed_data pdata, Command *cmd);
STATIC void *launch_command(void *cmd_info);
STATIC void ctcp_handle(Irc server, struct parsed_data pdata);

Irc irc_connect(const char *address, const char *port) {

	Irc server;

	// Minimum validity checks
	if (!strchr(address, '.'))
		return NULL;

	server = calloc_w(sizeof(*server));
	server->conn = sock_connect(address, port);
	if (server->conn < 0) {
		free(server);
		return NULL;
	}
	server->mtx = malloc_w(sizeof(*server->mtx));
	if (pthread_mutex_init(server->mtx, NULL))
		goto cleanup;

	if (pipe(server->pipe))
		goto cleanup;

	// Set socket to non-blocking mode
	if (fcntl(server->conn, F_SETFL, O_NONBLOCK))
		goto cleanup;

	strncpy(server->address, address, ADDRLEN);
	strncpy(server->port, port, PORTLEN);

	return server;

cleanup:
	perror(__func__);
	free(server->mtx);
	free(server);
	return NULL;
}

int get_socket(Irc server) {

	return server->conn;
}

void set_mqueue(Irc server, Mqueue mq) {

	assert(mq);
	server->mqueue = mq;
}

char *default_channel(Irc server) {

	if (!server->channels_set)
		return NULL;

	return server->channels[0];
}

STATIC bool user_is_identified(Irc server, const char *nick) {

	int auth_level = 0;

#ifndef NDEBUG
	extern pthread_t main_thread_id;
#endif

	// Avoid deadlock
	assert(!pthread_equal(main_thread_id, pthread_self()));

	pthread_mutex_lock(server->mtx);
	send_message(server, "NickServ", "ACC %s", nick);
	if (sock_read(server->pipe[RD], &auth_level, 4) != 4)
		perror(__func__);

	pthread_mutex_unlock(server->mtx);
	return auth_level == 3;
}

bool user_has_access(Irc server, const char *nick) {

	return user_in_access_list(nick) && user_is_identified(server, nick);
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

	snprintf(user_with_flags, sizeof(user_with_flags), "%s 0 * :%s", server->user, server->user);
	irc_command(server, "USER", user_with_flags);
}

int join_channel(Irc server, const char *channel) {

	int i = 0;

	// Join all channels if arg is NULL
	if (!channel) {
		if (server->connected)
			while (i < server->channels_set)
				irc_command(server, "JOIN", server->channels[i++]);

		return i;
	}
	assert(channel[0] == '#');
	for (i = 0; i < server->channels_set; i++)
		if (streq(channel, server->channels[i]))
			break;

	// Add channel if it's not a duplicate
	if (i == server->channels_set) {
		if (server->channels_set == MAXCHANS) {
			fprintf(stderr, "Channel limit reached (%d)\n", MAXCHANS);
			return 0;
		}
		strncpy(server->channels[server->channels_set++], channel, CHANLEN);
	}
	if (server->connected)
		irc_command(server, "JOIN", channel);

	return 1;
}

ssize_t parse_irc_line(Irc server) {

	int reply;
	ssize_t n;
	Command *cmd;
	struct parsed_data pdata;

	// Read raw line from server. Example: ":laxanofido!~laxanofid@snf-23545.vm.okeanos.grnet.gr PRIVMSG #foss-teimes :How YA doing fossbot"
	n = sock_readline(server->conn, server->line + server->line_offset, IRCLEN - server->line_offset);
	if (n <= 0) {
		if (n != -EAGAIN)
			exit_msg("IRC connection closed");

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
		numeric_reply(server, pdata, reply);
	else {
		// Find & launch any functions registered to IRC commands
		cmd = command_lookup(pdata.command, strlen(pdata.command));
		if (cmd)
			cmd->function(server, pdata);
	}
	return n;
}

void irc_privmsg(Irc server, struct parsed_data pdata) {

	Command *cmd;

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
		cmd = command_lookup(pdata.command, strlen(pdata.command));
		if (cmd)
			pre_launch_command(server, pdata, cmd);
	}
	// CTCP requests must begin with ascii char 1 (SOH - start of heading)
	else if (*pdata.command == '\x01')
		ctcp_handle(server, pdata);
}

STATIC void pre_launch_command(Irc server, struct parsed_data pdata, Command *cmd) {

	pthread_t id;
	struct command_info *cmdi;

	cmdi = malloc_w(sizeof(*cmdi));
	cmdi->line = malloc_w(IRCLEN + 1);
	memcpy(cmdi->line, server->line, IRCLEN + 1);
	cmdi->cmd = cmd;
	cmdi->server = server;

	cmdi->pdata.sender      = cmdi->line + (pdata.sender  - server->line);
	cmdi->pdata.command     = cmdi->line + (pdata.command - server->line);
	cmdi->pdata.target      = cmdi->line + (pdata.target  - server->line);
	if (pdata.message)
		cmdi->pdata.message = cmdi->line + (pdata.message - server->line);
	else
		cmdi->pdata.message = NULL;

	if (pthread_create(&id, NULL, launch_command, cmdi))
		perror("Could not launch command");
}

STATIC void *launch_command(void *cmd_info) {

	struct command_info *cmdi = cmd_info;

	pthread_detach(pthread_self());
	cmdi->cmd->function(cmdi->server, cmdi->pdata);

	free(cmdi->line);
	free(cmdi);
	return NULL;
}

STATIC void ctcp_handle(Irc server, struct parsed_data pdata) {

	time_t now;
	struct timeval tm;
	char *now_str, time_micro_str[64];

	now = time(NULL);
	now_str = ctime(&now);
	now_str[strlen(now_str) - 1] = '\0'; // Cut newline
	gettimeofday(&tm, NULL);
	snprintf(time_micro_str, 64, "%ld %ld", tm.tv_sec, tm.tv_usec);

	pdata.command++; // Skip the leading escape char
	if (starts_case_with(pdata.command, "VERSION"))
		ctcp_reply(server, pdata.sender, "VERSION %s", cfg.bot_version);
	else if (starts_case_with(pdata.command, "TIME"))
		ctcp_reply(server, pdata.sender, "TIME %s", now_str);
	else if (starts_case_with(pdata.command, "PING"))
		ctcp_reply(server, pdata.sender, "PING %s", pdata.message ? pdata.message : time_micro_str);
}

int numeric_reply(Irc server, struct parsed_data pdata, int reply) {

	int i;
	char newnick[NICKLEN];

	switch (reply) {
	case NICKNAMEINUSE: // Change our nick
		i = snprintf(newnick, NICKLEN, "%s_", server->nick);
		if (i >= NICKLEN)
			exit_msg("maximum nickname length reached");

		set_nick(server, newnick);
		break;
	case ENDOFMOTD: // Join all set channels
		server->connected = true;
		join_channel(server, NULL);
		break;
	case BANNEDFROMCHAN: // Find the channel we got banned and remove it from our list
		pdata.target = strtok(pdata.message, " ");
		if (!pdata.target)
			break;

		for (i = 0; i < server->channels_set; i++)
			if (streq(pdata.target, server->channels[i]))
				break;

		strncpy(server->channels[i], server->channels[--server->channels_set], CHANLEN);
		break;
	}
	return reply;
}

void irc_notice(Irc server, struct parsed_data pdata) {

	char *test;
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
		if (sock_write(server->pipe[WR], &auth_level, 4) != 4)
			perror(__func__);
	} else if (starts_with(pdata.message, "This nickname is registered") && *cfg.nick_password) {
		send_message(server, pdata.sender, "identify %s", cfg.nick_password);
		memset(cfg.nick_password, '\0', strlen(cfg.nick_password));
	}
}

void irc_kick(Irc server, struct parsed_data pdata) {

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
#ifndef TEST
		sleep(5);
#endif
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

#ifdef TEST
	sock_write(server->conn, irc_msg, strlen(irc_msg));
#else
	mqueue_send(server->mqueue, irc_msg);
#endif
}

void quit_server(Irc server, const char *msg) {

	char buf[QUITLEN + 1] = ":";

	assert(msg);
	strncat(buf, msg, QUITLEN - 1);
	irc_command(server, "QUIT", buf);

	if (close(server->conn))
		perror(__func__);

	if (pthread_mutex_destroy(server->mtx))
		perror(__func__);

	mqueue_destroy(server->mqueue);
	free(server->mtx);
	free(server);
}
