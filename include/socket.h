#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>

#define BUFSIZE  512

int sock_connect(const char *server, const char *port);
ssize_t sock_write(int sock, const char *buf, size_t n);
ssize_t sock_readbyte(int sock, char *byte);
ssize_t sock_readline(int sock, char *line_buf, size_t len);

#endif