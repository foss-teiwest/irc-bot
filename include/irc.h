#ifndef IRC_H
#define IRC_H

#include <sys/types.h>

#define BUFSIZE  512
#define NICKLEN  16
#define USERLEN  15
#define CHANLEN  40
#define SERVLEN  40
#define PORTLEN  5

enum { Freenode, Grnet, Testing };
typedef struct irc_type *Irc;
typedef struct parse_type {
	char *nick;
	char *command;
	char *target;
	char *message;
} *Parsed_data;

// Fill server details with the one specified and connect to it.
// Structure returned is allocated on the heap so it needs to be freed with quit_server()
// Returns NULL on failure
Irc connect_server(int server_list);

// The (char *) in the functions that return it, is the final message composed for sending to server

// If NULL is entered then the default server value is used
// User can only be set during server connection so it should only be called once
char *set_nick(Irc server, const char *nick);
char *set_user(Irc server, const char *user);
char *join_channel(Irc server, const char *channel);

// Gets next line from server. Returns length
ssize_t get_line(Irc server, char *buf);

// Change second character of PING request, from 'I' to 'O' and send it back
char *ping_reply(Irc server, char *buf);

// Split line into Parsed_data structure elements and launch the function associated with IRC commands
// The function called gets the parameters after command, to parse them itself
void parse_line(Irc server, char *line, Parsed_data pdata);

// Parse channel / private messages and launch the function that matches the bot command. Must begin with '!'
// Info available in pdata: nick, command, message (the rest message after command, including target)
void irc_privmsg(Irc server, Parsed_data pdata);

// Send a message to a channel or a person specified by target. Standard printf format accepted
char *send_message(Irc server, const char *target, const char *format, ...);

// Close socket and free structure. Returns -1 on failure
char *quit_server(Irc server, const char *msg);

#endif