#ifndef IRC_H
#define IRC_H

#include <sys/types.h>

#define IRCLEN   512
#define NICKLEN  20
#define USERLEN  15
#define CHANLEN  40
#define ADDRLEN  40
#define PORTLEN  5
#define MAXCHANS 2
#define TIMEOUT  360

typedef struct irc_type *Irc;
typedef struct {
	char *sender;
	char *command;
	char *target;
	char *message;
} Parsed_data;

enum irc_reply {
	ENDOFMOTD     = 376,
	NICKNAMEINUSE = 433
};

// Connect to the server specified and initialize curl library
// Structure returned is allocated on the heap so it needs to be freed with quit_server()
// Returns NULL on failure
Irc connect_server(const char *address, const char *port);

// User can only be set during server connection so it should only be called once
void set_nick(Irc server, const char *nick);
void set_user(Irc server, const char *user);
void set_channel(Irc server, const char *channel);

// If channel argument is NULL then join all channels registered with set_channel()
int join_channel(Irc server, const char *channel);

// Read line from server, split it into Parsed_data structure elements and launch the function associated with IRC commands
ssize_t parse_line(Irc server);

// Parse channel / private messages and launch the function that matches the BOT command (must begin with '!') or CTCP request.
// Info available in pdata: nick, command, message (the rest message after command, including target)
void irc_privmsg(Irc server, Parsed_data pdata);

// Handle notices. If for example nick requires identify, you will be prompted to enter it in standard input
void irc_notice(Irc server, Parsed_data pdata);

// Rejoin 5 secs after being kicked and send message to offender
void irc_kick(Irc server, Parsed_data pdata);

// Handle server numeric replies
int numeric_reply(Irc Server, int reply);

// Wrappers to send message / notice to target channel / person. Standard printf format accepted. Do not call _send_irc_command() directly
// Example: "send_message(server, pdata.target, "hi %s", pdata->sender);"
#define send_message(server, target, ...) _send_irc_command(server, "PRIVMSG", target, __VA_ARGS__)
#define send_notice(server, target, ...)  _send_irc_command(server, "NOTICE", target, __VA_ARGS__)
void _send_irc_command(Irc server, const char *type, const char *target, ...);

// Close socket, cleanup curl library and free resources
void quit_server(Irc server, const char *msg);

#endif
