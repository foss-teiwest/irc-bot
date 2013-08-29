#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>

/**
 * @file socket.h
 * Contains low level read & write operations
 */

#define IRCLEN 512

/**
 * Open a connection to a remote location
 *
 * @param address  hostname / IP
 * @param port     Port in string form
 * @returns        A valid socket descriptor or -1 on error
 */
int sock_connect(const char *address, const char *port);

/**
 * Write the input unaltered to socket descriptor
 *
 * @param sock  Non blocking socket
 * @param buf   Buffer to send
 * @param len   Number of bytes to write
 * @returns     On success: the amount of bytes written (this will always equal len), -1 on error and -EAGAIN if the operation would block
 */
ssize_t sock_write(int sock, const char *buf, size_t len);

/**
 * Read a valid IRC line containing "\r\n" at the end and null terminate it
 * Characters after the IRC message terminators are left untouched for the next call
 * @warning NOT thread safe. Mantains it's own buffer internally to avoid calling single byte read's()
 *
 * @param sock      Non blocking socket
 * @param line_buf  Buffer to store the line
 * @param len       Will fill the buffer up to len bytes
 * @returns         On success: line length, -1 on error and -EAGAIN if the operation would block
 */
ssize_t sock_readline(int sock, char *line_buf, size_t len);

#endif
