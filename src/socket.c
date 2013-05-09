#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include "socket.h"
#include "wrapper.h"
#include <assert.h>

int sock_connect(const char *server, const char *port) {

	int sock, ret_value;
	struct addrinfo addr_filter, *addr_holder, *addr_iterator;

	// Create filter for getaddrinfo()
	memset(&addr_filter, 0, sizeof(addr_filter));
	addr_filter.ai_family   = AF_UNSPEC; 	   // IPv4 or IPv6
	addr_filter.ai_socktype = SOCK_STREAM; 	   // Stream socket
	addr_filter.ai_protocol = IPPROTO_TCP;	   // TCP protocol
	// Don't try to resolve service -> port, since we already provide it in numeric form
	addr_filter.ai_flags   |= AI_NUMERICSERV;

	// Return addresses according to the filter criteria
	ret_value = getaddrinfo(server, port, &addr_filter, &addr_holder);
	if (ret_value != 0)
		exit_msg_details("getaddrinfo", gai_strerror(ret_value));

	sock = -1;
	for (addr_iterator = addr_holder; addr_iterator != NULL; addr_iterator = addr_holder->ai_next) {
		// Create TCP socket
		sock = socket(addr_iterator->ai_family, addr_iterator->ai_socktype, addr_iterator->ai_protocol);
		if (sock < 0)
			continue; // Failed, try next address

		ret_value = connect(sock, addr_iterator->ai_addr, addr_iterator->ai_addrlen);
		if (ret_value == 0)
			break; // Success

		close(sock); // Cleanup and try next address
		sock = -1;
	}
	freeaddrinfo(addr_holder);
	return sock;
}

ssize_t sock_write(int sock, const char *buf, size_t n) {

	ssize_t n_sent;
	size_t n_left = n;
	const char *buf_marker = buf;

	assert(n <= strlen(buf) && "Write length is bigger than buffer size");

	while (n_left > 0) {
		n_sent = write(sock, buf_marker, n_left);
		if (n_sent < 0 && errno == EINTR) // Interrupted by signal, retry
			n_sent = 0;
		else if (n_sent < 0) {
			exit_errno("write");
			return -1;
		}
		n_left -= n_sent;
		buf_marker += n_sent; // Advance buffer pointer to the next unsent bytes
	}
	return n;
}