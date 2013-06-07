#ifndef IRC_H
#define IRC_H

#include <sys/types.h>

#define IRCLEN  512
#define NICKLEN  16
#define USERLEN  15
#define CHANLEN  40
#define ADDRLEN  40
#define PORTLEN  5

typedef struct irc_type *Irc;
typedef struct {
	char *sender;
	char *command;
	char *target;
	char *message;
} *Parsed_data;

enum { NICKNAMEINUSE = 433 };

// Fill server details with the one specified and connect to it.
// Structure returned is allocated on the heap so it needs to be freed with quit_server()
// Returns NULL on failure
Irc connect_server(const char *address, const char *port);

// If NULL is entered then the default server value is used
// User can only be set during server connection so it should only be called once
// Returns the final account info set
char *set_nick(Irc server, const char *nick);
char *set_user(Irc server, const char *user);
char *join_channel(Irc server, const char *channel);

// pwd is cleared after use so it doesn't stay in plain view (memory)
// And also the command sent to server is not printed in stdout
void identify_nick(Irc server, char *pwd);

// Gets next line from server. Returns length
ssize_t get_line(Irc server, char *buf);

// Change second character of PING request, from 'I' to 'O' and send it back
char *ping_reply(Irc server, char *buf);

// Split line into Parsed_data structure elements and launch the function associated with IRC commands
// The function called gets the parameters after command, to parse them itself
char *parse_line(Irc server, char *line, Parsed_data pdata);

// Parse channel / private messages and launch the function that matches the bot command. Must begin with '!'
// Info available in pdata: nick, command, message (the rest message after command, including target)
void irc_privmsg(Irc server, Parsed_data pdata);

// Handle server numeric replies
int numeric_reply(Irc Server, int reply);

// Send a message to a channel or a person specified by target. Standard printf format accepted
void send_message(Irc server, const char *target, const char *format, ...);

// Close socket and free structure. Returns -1 on failure
void quit_server(Irc server, const char *msg);

#endif