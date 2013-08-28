#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <assert.h>
#include "socket.h"
#include "helper.h"


int sock_connect(const char *address, const char *port) {

	int sock, retval;
	struct addrinfo addr_filter, *addr_holder, *addr_iterator;

	// Create filter for getaddrinfo()
	memset(&addr_filter, 0, sizeof(addr_filter));
	addr_filter.ai_family   = AF_UNSPEC;    // IPv4 or IPv6
	addr_filter.ai_socktype = SOCK_STREAM;  // Stream socket
	addr_filter.ai_protocol = IPPROTO_TCP;  // TCP protocol

	// Don't try to resolve service -> port, since we already provide it in numeric form
	addr_filter.ai_flags   |= AI_NUMERICSERV;

	// Return addresses according to the filter criteria
	if ((retval = getaddrinfo(address, port, &addr_filter, &addr_holder)) != 0)
		exit_msg("getaddrinfo: %s", gai_strerror(retval));

	sock = -1;
	for (addr_iterator = addr_holder; addr_iterator != NULL; addr_iterator = addr_holder->ai_next) {

		// Create TCP socket
		if ((sock = socket(addr_iterator->ai_family, addr_iterator->ai_socktype, addr_iterator->ai_protocol)) < 0)
			continue; // Failed, try next address

		if ((retval = connect(sock, addr_iterator->ai_addr, addr_iterator->ai_addrlen)) == 0)
			break; // Success

		close(sock); // Cleanup and try next address
		sock = -1;
	}
	freeaddrinfo(addr_holder);
	return sock;
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
			return errno == EAGAIN ? -2 : (perror("write"), -1);
		n_left -= n_sent;
		buf_marker += n_sent; // Advance buffer pointer to the next unsent byte
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
	// -2 if the operation would block and 0 if connection is closed
	if (bytes_read <= 0) {
		if ((bytes_read = read(sock, buffer, IRCLEN)) <= 0)
			return errno == EAGAIN ? -2 : (perror("read"), bytes_read);
		buf_ptr = buffer;
	}
	bytes_read--;
	*byte = *buf_ptr++;

	return 1;
}

ssize_t sock_readline(int sock, char *line_buf, size_t len) {

	size_t n_read = 0;
	ssize_t n;
	char byte;

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
