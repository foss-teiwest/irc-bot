#ifndef IRC_H
#define IRC_H

#include <stdbool.h>

#define BUFSIZE  512
#define NICKLEN  16
#define USERLEN  15
#define CHANLEN  40
#define SERVLEN  40
#define PORTLEN  5

typedef struct irc_type *Irc;
typedef enum { Freenode } Server_list;
Server_list server_list;

// Fills server info to the Irc data type according to the server selected
// Structure returned is malloc'ed so it needs to be free'd with irc_quit_server()
Irc irc_select_server(Server_list server_list);

// Connect to the server specified by the Irc struct, set nick, user and join channel
// Returns -1 on socket error and -2 if irc server doesn't respond
int irc_connect_server(Irc server);

// If NULL is entered in nick, user and channel functions, then the value is read from the server struct
// set_nick returns -1 if nickname exists
int irc_set_nick(Irc server, const char *nick);
void irc_set_user(Irc server, const char *user);
void irc_join_channel(Irc server, const char *channel);
bool irc_is_connected(Irc server);

// Close socket and free structure
void irc_quit_server(Irc server);

#endif