#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>

int sock_connect(const char *server, const char *port);
ssize_t sock_write(int sock, const char *buf, size_t n);

#endif