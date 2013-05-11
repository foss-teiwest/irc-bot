#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>

#define BUFSIZE  512

// Returns a valid socket descriptor or -1 if it fails
int sock_connect(const char *address, const char *port);

// Write n bytes from buf without altering them (raw)
// Returns the length specified (n) or -1 if it fails
ssize_t sock_write(int sock, const char *buf, size_t n);

// Mantain our own buffer so we don't call read() for every single byte
// Stores the character in byte. Returns -1 for fail, 0 if connection is closed or 1 for success
ssize_t sock_readbyte(int sock, char *byte);

// Build a valid IRC line (with \r\n terminators) and store it null terminated in line_buff
// Returns line length. Characters after the IRC message terminators are left untouched for the next call
ssize_t sock_readline(int sock, char *line_buf, size_t len);

#endif