#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include "socket.h"
#include "common.h"


int sock_connect(const char *address, const char *port) {

	int retval, sock = -1;
	struct addrinfo addr_filter, *addr_holder, *addr_iterator;

	// Create filter for getaddrinfo()
	memset(&addr_filter, 0, sizeof(addr_filter));
	addr_filter.ai_family   = AF_UNSPEC;      // IPv4 or IPv6
	addr_filter.ai_socktype = SOCK_STREAM;    // Stream socket
	addr_filter.ai_protocol = IPPROTO_TCP;    // TCP protocol
	addr_filter.ai_flags    = AI_NUMERICSERV; // Don't resolve service -> port, since we already provide it in numeric form

	// Return addresses according to the filter criteria
	retval = getaddrinfo(address, port, &addr_filter, &addr_holder);
	if (retval) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retval));
		return sock;
	}
	for (addr_iterator = addr_holder; addr_iterator; addr_iterator = addr_iterator->ai_next) {

		sock = socket(addr_iterator->ai_family, addr_iterator->ai_socktype, addr_iterator->ai_protocol);
		if (sock < 0) {
			perror(__func__);
			continue; // Failed, try next address
		}
		if (!connect(sock, addr_iterator->ai_addr, addr_iterator->ai_addrlen))
			break; // Success

		// Cleanup and try next address
		perror(__func__);
		close(sock);
		sock = -1;
	}
	freeaddrinfo(addr_holder);
	return sock;
}

int sock_listen(const char *address, const char *port) {

	int retval, sock = -1;
	struct addrinfo addr_filter, *addr_holder, *addr_iterator;

	addr_filter.ai_family   = AF_UNSPEC;
	addr_filter.ai_socktype = SOCK_STREAM;
	addr_filter.ai_protocol = IPPROTO_TCP;
	addr_filter.ai_flags    = AI_NUMERICSERV;

	retval = getaddrinfo(address, port, &addr_filter, &addr_holder);
	if (retval) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retval));
		return sock;
	}
	for (addr_iterator = addr_holder; addr_iterator; addr_iterator = addr_iterator->ai_next) {

		sock = socket(addr_iterator->ai_family, addr_iterator->ai_socktype, addr_iterator->ai_protocol);
		if (sock < 0) {
			perror(__func__);
			continue;
		}
		// Allow us to re-use the binding port
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int) { 1 }, sizeof(int));
		if (!bind(sock, addr_iterator->ai_addr, addr_iterator->ai_addrlen) && !listen(sock, 5))
			break; // Success

		perror(__func__);
		close(sock);
		sock = -1;
	}
	freeaddrinfo(addr_holder);
	return sock;
}

int sock_accept(int listen_fd, bool non_block) {

	int accept_fd;

	accept_fd = accept(listen_fd, NULL, NULL);
	if (accept_fd < 0) {
		perror(__func__);
		return -1;
	}
	if (non_block)
		fcntl(accept_fd, F_SETFL, O_NONBLOCK);

	return accept_fd;
}

ssize_t sock_write(int sock, const void *buffer, size_t len) {

	ssize_t n_sent;
	size_t n_left = len;
	const unsigned char *buf = buffer;

	while (n_left > 0) {
		n_sent = write(sock, buf, n_left);
		if (n_sent < 0) {
			if (errno == EINTR) // Interrupted by signal, retry
				continue;

			perror(__func__);
			return -1;
		}
		n_left -= n_sent;
		buf += n_sent; // Advance buffer pointer to the next unsent bytes
	}
	return len;
}

ssize_t sock_write_non_blocking(int sock, const void *buffer, size_t len) {

	const unsigned char *buf = buffer;

	// TODO we don't actually resend the buffer if we get EAGAIN...
	if (write(sock, buf, len) < 0)
		return errno == EAGAIN ? -EAGAIN : (perror(__func__), -1);

	return len;
}

ssize_t sock_read(int sock, void *buffer, size_t len) {

	ssize_t n;
	unsigned char *buf = buffer;

	while (true) {
		n = read(sock, buf, len);
		if (n == -1 && errno == EINTR)
			continue;

		if (n == -1)
			perror(__func__);

		return n;
	}
}

ssize_t sock_read_non_blocking(int sock, void *buffer, size_t len) {

	ssize_t n;
	unsigned char *buf = buffer;

	n = read(sock, buf, len);
	if (n == -1 && errno == EAGAIN)
		return -EAGAIN;

	if (n == -1)
		perror(__func__);

	return n;
}

STATIC ssize_t sock_readbyte(int sock, char *byte) {

	static ssize_t bytes_read;
	static char buffer[IRCLEN];
	static char *buf_ptr;

	// Stores the character in byte. Returns 1 on success, -1 on error,
	// -EAGAIN if the operation would block and 0 if connection is closed
	if (bytes_read <= 0) {
		bytes_read = read(sock, buffer, IRCLEN);
		if (bytes_read <= 0) {
			if (bytes_read == -1 && errno == EAGAIN)
				return -EAGAIN;

			if (bytes_read == -1)
				perror(__func__);

			return bytes_read;
		}
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

	// If n == 0, connection is closed. Return bytes read so far
	while (n_read++ <= len) {
		n = sock_readbyte(sock, &byte);
		if (n <= 0) {
			*line_buf = '\0';
			return n == 0 ? (ssize_t) n_read - 1 : n;
		}
		*line_buf++ = byte;
		if (byte == '\n' && *(line_buf - 2) == '\r')
			break; // Message complete, we found irc protocol terminators
	}
	*line_buf = '\0';
	return n_read;
}
