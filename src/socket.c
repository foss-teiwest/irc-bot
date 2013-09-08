#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <assert.h>
#include "socket.h"
#include "helper.h"


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
	if ((retval = getaddrinfo(address, port, &addr_filter, &addr_holder)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retval));
		return sock;
	}

	for (addr_iterator = addr_holder; addr_iterator != NULL; addr_iterator = addr_iterator->ai_next) {

		if ((sock = socket(addr_iterator->ai_family, addr_iterator->ai_socktype, addr_iterator->ai_protocol)) < 0) {
			perror("socket");
			continue; // Failed, try next address
		}
		if (connect(sock, addr_iterator->ai_addr, addr_iterator->ai_addrlen) == 0)
			break; // Success

		// Cleanup and try next address
		perror("connect");
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

	if ((retval = getaddrinfo(address, port, &addr_filter, &addr_holder)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retval));
		return sock;
	}

	for (addr_iterator = addr_holder; addr_iterator != NULL; addr_iterator = addr_iterator->ai_next) {

		if ((sock = socket(addr_iterator->ai_family, addr_iterator->ai_socktype, addr_iterator->ai_protocol)) < 0) {
			perror("socket");
			continue;
		}
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int) { 1 }, sizeof(int));
		if (bind(sock, addr_iterator->ai_addr, addr_iterator->ai_addrlen) == 0 && listen(sock, 5) == 0)
			break; // Success

		perror("bind/listen");
		close(sock);
		sock = -1;
	}
	freeaddrinfo(addr_holder);
	return sock;
}

int sock_accept(int listen_fd) {

	int accept_fd;

	if ((accept_fd = accept(listen_fd, NULL, NULL)) < 0) {
		perror("accept");
		return -1;
	}
	return accept_fd;
}

ssize_t sock_write(int sock, const char *buf, size_t len) {

	ssize_t n_sent;
	size_t n_left;
	const char *buf_marker;

	assert(len <= strlen(buf) && "Write length is bigger than buffer size");
	n_left = len;
	buf_marker = buf;

	// TODO we don't actually resend the buffer if we get EAGAIN...
	while (n_left > 0) {
		if ((n_sent = write(sock, buf_marker, n_left)) < 0)
			return errno == EAGAIN ? -EAGAIN : (perror("write"), -1);

		n_left -= n_sent;
		buf_marker += n_sent; // Advance buffer pointer to the rest unsent bytes
	}
	return len;
}

#ifdef TEST
	ssize_t sock_readbyte(int sock, char *byte)
#else
	static ssize_t sock_readbyte(int sock, char *byte)
#endif
{
	static ssize_t bytes_read;
	static char buffer[IRCLEN];
	static char *buf_ptr;

	// Stores the character in byte. Returns 1 on success, -1 on error,
	// -EAGAIN if the operation would block and 0 if connection is closed
	if (bytes_read <= 0) {
		if ((bytes_read = read(sock, buffer, IRCLEN)) <= 0)
			return errno == EAGAIN ? -EAGAIN : (perror("read"), bytes_read);

		buf_ptr = buffer;
	}
	bytes_read--;
	*byte = *buf_ptr++;

	return 1;
}

ssize_t sock_readline(int sock, char *line_buf, size_t len) {

	size_t n_read = 0;
	ssize_t n;
	char byte = '\0';

	// If n == 0, connection is closed. Return bytes read so far
	while (n_read++ <= len) {
		if ((n = sock_readbyte(sock, &byte)) <= 0) {
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
