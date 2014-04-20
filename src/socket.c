#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include "socket.h"
#include "irc.h"
#include "common.h"

int sock_connect(const char *address, const char *port) {

	int retval, sock = -1;
	struct addrinfo *addr, *iterator;

	// Set hints for getaddrinfo()
	struct addrinfo hints = {
		.ai_family   = AF_UNSPEC,      // IPv4 or IPv6
		.ai_socktype = SOCK_STREAM,    // Stream socket / TCP protocol
		.ai_flags    = AI_NUMERICSERV  // Don't resolve service -> port, since we already provide it in numeric form
	};
	assert(atoi(port) > 0 && atoi(port) <= MAXPORT);

	// Return addresses according to the hints specified
	retval = getaddrinfo(address, port, &hints, &addr);
	if (retval) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retval));
		return -1;
	}
	for (iterator = addr; iterator; iterator = iterator->ai_next) {

		sock = socket(iterator->ai_family, iterator->ai_socktype, iterator->ai_protocol);
		if (sock < 0) {
			perror(__func__);
			continue; // Failed, try next address
		}
		if (!connect(sock, iterator->ai_addr, iterator->ai_addrlen))
			break; // Success

		// Cleanup and try next address
		perror(__func__);
		close(sock);
		sock = -1;
	}
	freeaddrinfo(addr);
	return sock;
}

int sock_listen(const char *address, const char *port) {

	int retval, sock = -1;
	struct addrinfo *addr, *iterator;

	struct addrinfo hints = {
		.ai_family   = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM,
		.ai_flags    = AI_NUMERICSERV
	};
	assert(atoi(port) > 0 && atoi(port) <= MAXPORT);

	retval = getaddrinfo(address, port, &hints, &addr);
	if (retval) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retval));
		return -1;
	}
	for (iterator = addr; iterator; iterator = iterator->ai_next) {

		sock = socket(iterator->ai_family, iterator->ai_socktype, iterator->ai_protocol);
		if (sock < 0) {
			perror(__func__);
			continue;
		}
		// Allow us to re-use the binding port
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(socklen_t) {1}, sizeof(socklen_t));
		if (!bind(sock, iterator->ai_addr, iterator->ai_addrlen) && !listen(sock, 5))
			break; // Success

		perror(__func__);
		close(sock);
		sock = -1;
	}
	freeaddrinfo(addr);
	return sock;
}

int sock_accept(int listen_fd, bool non_block) {

	int accept_fd;

	accept_fd = accept(listen_fd, NULL, NULL);
	if (accept_fd < 0) {
		perror(__func__);
		return -1;
	}
	if (non_block && fcntl(accept_fd, F_SETFL, O_NONBLOCK)) {
		perror(__func__);
		close(accept_fd);
		return -1;
	}
	return accept_fd;
}

ssize_t sock_write(int sock, const void *buffer, size_t len) {

	ssize_t n_sent;
	size_t n_left = len;
	const unsigned char *buf = buffer;

	while (n_left > 0) {
		n_sent = write(sock, buf, n_left);
		if (n_sent == -1) {
			if (errno == EINTR) // Interrupted by signal, retry
				continue;

			if (errno == EAGAIN) // Operation would block, return bytes written so far
				return len - n_sent;

			perror(__func__);
			return -1;
		}
		n_left -= n_sent;
		buf += n_sent; // Advance buffer pointer to the next unsent bytes
	}
	return len;
}

ssize_t sock_read(int sock, void *buffer, size_t len) {

	ssize_t n;
	unsigned char *buf = buffer;

	while (true) {
		n = read(sock, buf, len);
		if (n == -1) {
			if (errno == EINTR)
				continue;

			if (errno == EAGAIN)
				return -EAGAIN;

			perror(__func__);
		}
		return n;
	}
}

STATIC ssize_t sock_readbyte(int sock, char *byte) {

	static ssize_t bytes_read;
	static char buffer[IRCLEN];
	static char *buf_ptr;

	// Stores the character in byte. Returns 1 on success, -1 on error,
	// -EAGAIN if the operation would block and 0 if connection is closed
	if (bytes_read <= 0) {
		bytes_read = sock_read(sock, buffer, IRCLEN);
		if (bytes_read <= 0)
			return bytes_read;

		buf_ptr = buffer;
	}
	bytes_read--;
	*byte = *buf_ptr++;

	return 1;
}

ssize_t sock_readline(int sock, char *line_buf, size_t len) {

	ssize_t n;
	size_t n_read = 0;
	char byte = '\0';

	while (n_read++ <= len) {
		n = sock_readbyte(sock, &byte);
		if (n <= 0) {
			*line_buf = '\0';
			if (n == 0) // connection closed. Return bytes read so far
				return n_read - 1;

			return n;
		}
		*line_buf++ = byte;
		if (byte == '\n' && *(line_buf - 2) == '\r')
			break; // Message complete, we found irc protocol terminators
	}
	*line_buf = '\0';
	return n_read;
}
