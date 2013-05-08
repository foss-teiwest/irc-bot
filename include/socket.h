#ifndef SOCKET_H
#define SOCKET_H

#define SERVER  "chat.freenode.net"
#define PORT 	"6667"
#define NICK	"botten_anna"

int sock_connect(const char *server, const char *port);
ssize_t sock_write(int sock, const char *buf, size_t n);

#endif