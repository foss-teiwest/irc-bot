#ifndef SOCKET_H
#define SOCKET_H

/**
 * @file socket.h
 * Contains low level read & write operations
 */

#include <sys/types.h>
#include <stdbool.h>

#define NONBLOCK 1
#define IRCLEN   512
#define MAXPORT  65535
#define LOCALHOST "127.0.0.1"

/**
 * Open a connection to a remote location
 *
 * @param address  hostname / IP
 * @param port     Port in string form
 * @returns        A valid socket descriptor or -1 on error
 */
int sock_connect(const char *address, const char *port);

/**
 * Start listening for connections
 *
 * @param address  hostname / IP
 * @param port     Port in string form
 * @returns        A valid socket descriptor or -1 on error
 */
int sock_listen(const char *address, const char *port);

/**
 * Accept connection on specified socket
 *
 * @param listen_fd  From which socket to accept connection
 * @param non_block  If true, it will set accepted socket to non-blocking mode
 * @returns          A valid socket descriptor or -1 on error
 */
int sock_accept(int listen_fd, bool non_block);

/**
 * Write the input unaltered to socket descriptor
 *
 * @param sock    (Non) blocking socket
 * @param buffer  Buffer to send
 * @param len     Number of bytes to write
 * @returns       blocking: On success: the amount of bytes written (this will always equal len) or -1 on error
 *                non-blocking: If the return value != len then a partial write was made due to EAGAIN
 */
ssize_t sock_write(int sock, const void *buffer, size_t len);

/**
 * Read data into buffer
 *
 * @param sock    (Non) blocking socket
 * @param buffer  Buffer to store data
 * @param len     Number of bytes to read
 * @returns       blocking: On success: the amount of bytes read, -1 on error and 0 if connection is closed
 *                non-blocking: Same as blocking plus -EAGAIN if the operation would block
 */
ssize_t sock_read(int sock, void *buffer, size_t len);

/**
 * Read a valid IRC line containing "\r\n" at the end and null terminate it
 * Characters after the IRC message terminators are left untouched for the next call
 * In non blocking mode line_buffer will contain the chars read so far even if -EAGAIN is returned
 * The length of the line can be used as an offset for the next call till a positive return value
 * @warning NOT thread safe. Mantains it's own buffer internally to avoid calling single byte read's(). Als
 *
 * @param sock         Non blocking socket
 * @param line_buffer  Buffer to store the line
 * @param len          Will fill the buffer up to len bytes
 * @returns            On success: Amount of bytes written in line, -1 on error and -EAGAIN if the operation would block
 */
ssize_t sock_readline(int sock, char *line_buffer, size_t len);

#endif

