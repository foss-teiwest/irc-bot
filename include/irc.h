#ifndef IRC_H
#define IRC_H

#include <stdbool.h>
#include <sys/types.h>

#define BUFSIZE  512
#define NICKLEN  16
#define USERLEN  15
#define CHANLEN  40
#define SERVLEN  40
#define PORTLEN  5

typedef struct irc_type *Irc;
typedef enum { Freenode, Grnet } Server_list;
Server_list server_list;

// Fills server info to the Irc data type according to the server selected
// Structure returned is malloc'ed so it needs to be free'd with irc_quit_server()
Irc select_server(Server_list server_list);

// Connect to the server specified by the Irc struct, set nick, user and join channel
// Returns -1 if it fails
int connect_server(Irc server);

// If NULL is entered in nick, user or channel functions, then the value is read from the server struct
void set_nick(Irc server, const char *nick);
void set_user(Irc server, const char *user);
void join_channel(Irc server, const char *channel);

// Change second character of PING request, from 'I' to 'O' and send it back
void ping_reply(Irc server, char *buf);

// Gets next line from server. Returns length
ssize_t get_line(Irc server, char *buf);

// Close socket and free structure
void quit_server(Irc server);

#endif