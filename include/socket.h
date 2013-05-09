#ifndef SOCKET_H
#define SOCKET_H

int sock_connect(const char *server, const char *port);
ssize_t sock_write(int sock, const char *buf, size_t n);

#endif