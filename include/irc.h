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
enum Server_list { Freenode } server_list;

Irc irc_select_server(enum Server_list server_list);
int irc_connect_server(Irc server);
int irc_set_nick(Irc server, const char *nick);
void irc_set_user(Irc server, const char *user);
void irc_join_channel(Irc server, const char *channel);
bool irc_is_connected(Irc server);
void irc_quit_server(Irc server);

#endif