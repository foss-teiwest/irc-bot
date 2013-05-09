#ifndef IRC_H
#define IRC_H

#include <stdbool.h>

#define SERVER  "chat.freenode.net"
#define PORT 	"6667"
#define NICK	"botten_anna"
#define USER	"bot"
#define CHAN 	"randomblabla"

#define NICKLEN	16
#define USERLEN	12
#define CHANLEN 40
#define BUFSIZE 512
#define	MAXCHANNELS	1

typedef struct irc_type *Irc;

Irc irc_connect(int sock, const char *nick, const char *user);
bool irc_nick(Irc server, const char *nick);
void irc_user(Irc server, const char *user);
void irc_join(Irc server, const char *channel);
bool irc_connected(Irc server);

#endif