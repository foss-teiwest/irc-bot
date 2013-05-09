#ifndef IRC_H
#define IRC_H

#define SERVER  "chat.freenode.net"
#define PORT 	"6667"
#define NICK	"botten_anna"
#define USER	"bot"

#define NICKLEN	16
#define USERLEN	12
#define CHANLEN 40
#define	MAXCHANNELS	1

typedef struct irc_type *Irc;

Irc irc_connect(int sock, const char *nick, const char *user);

#endif