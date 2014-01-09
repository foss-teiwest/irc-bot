#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <errno.h>
#include "socket.h"
#include "irc.h"
#include "murmur.h"
#include "common.h"


STATIC int murmur_connect(const char *port) {

	int murmfd;
	unsigned char read_buffer[READ_BUFFER_SIZE];

	const unsigned char ice_isA_packet[] = {
		0x49, 0x63, 0x65, 0x50, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x01, 0x00,
		0x00, 0x00, 0x04, 0x4d, 0x65, 0x74, 0x61, 0x00, 0x00, 0x07, 0x69, 0x63, 0x65, 0x5f, 0x69, 0x73,
		0x41, 0x01, 0x00, 0x15, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0e, 0x3a, 0x3a, 0x4d, 0x75, 0x72, 0x6d,
		0x75, 0x72, 0x3a, 0x3a, 0x4d, 0x65, 0x74, 0x61
	};

	murmfd = sock_connect(LOCALHOST, port);
	if (murmfd < 0)
		return -1;

	if (sock_read(murmfd, read_buffer, READ_BUFFER_SIZE) != VALIDATE_CONNECTION_PACKET_SIZE) {
		fprintf(stderr, "Error: Failed to receive validate_packet\n");
		goto cleanup;
	}
	if (sock_write(murmfd, ice_isA_packet, sizeof(ice_isA_packet)) < 0) {
		fprintf(stderr, "Error: Failed to send ice_isA_packet\n");
		goto cleanup;
	}
	if (sock_read(murmfd, read_buffer, READ_BUFFER_SIZE) != ICE_ISA_REPLY_PACKET_SIZE) {
		fprintf(stderr, "Error: Failed to receive ice_isA_packet success reply\n");
		goto cleanup;
	}
	return murmfd; // Everything succeeded

cleanup:
	close(murmfd);
	return -1;
}

bool add_murmur_callbacks(const char *port) {

	uint16_t listener_port = htons(CB_LISTEN_PORT);
	unsigned char *listener_port_bytes = (unsigned char *) &listener_port;
	unsigned char read_buffer[READ_BUFFER_SIZE];
	int murm_callbackfd;

	const unsigned char addCallback_packet[] = {
		0x49, 0x63, 0x65, 0x50, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x02, 0x00,
		0x00, 0x00, 0x01, 0x31, 0x01, 0x73, 0x00, 0x0b, 0x61, 0x64, 0x64, 0x43, 0x61, 0x6c, 0x6c, 0x62,
		0x61, 0x63, 0x6b, 0x00, 0x00, 0x4b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x24, 0x34, 0x45, 0x35, 0x42,
		0x38, 0x42, 0x31, 0x37, 0x2d, 0x44, 0x43, 0x33, 0x38, 0x2d, 0x34, 0x31, 0x42, 0x38, 0x2d, 0x39,
		0x45, 0x30, 0x45, 0x2d, 0x35, 0x44, 0x34, 0x41, 0x43, 0x43, 0x30, 0x42, 0x30, 0x37, 0x46, 0x46,
		0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x19, 0x00, 0x00, 0x00, 0x01, 0x00, 0x09, 0x31, 0x32,
		0x37, 0x2e, 0x30, 0x2e, 0x30, 0x2e, 0x31, listener_port_bytes[1], listener_port_bytes[0], 0x00,
		0x00, 0xff, 0xff, 0xff, 0xff, 0x00
	};

	murm_callbackfd = murmur_connect(port);
	if (murm_callbackfd < 0)
		return false;

	if (sock_write(murm_callbackfd, addCallback_packet, sizeof(addCallback_packet)) < 0) {
		fprintf(stderr, "Error: Failed to send addCallback_packet\n");
		goto cleanup;
	}
	if (sock_read(murm_callbackfd, read_buffer, READ_BUFFER_SIZE) != ADDCALLBACK_REPLY_PACKET_SIZE) {
		fprintf(stderr, "Error: Failed to receive addCallback_packet success reply\n");
		goto cleanup;
	}
	return true; // Success

cleanup:
	close(murm_callbackfd);
	return false;
}

char *fetch_murmur_users(void) {

	int murmfd, bytes_written, user_counter = 0;
	unsigned char read_buffer[USERLIST_BUFFER_SIZE];
	unsigned char *username = read_buffer + 50;
	char *user_list;

	const unsigned char getUsers_packet[] = {
		0x49, 0x63, 0x65, 0x50, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x07, 0x00,
		0x00, 0x00, 0x01, 0x31, 0x01, 0x73, 0x00, 0x08, 0x67, 0x65, 0x74, 0x55, 0x73, 0x65, 0x72, 0x73,
		0x02, 0x00, 0x06, 0x00, 0x00, 0x00, 0x01, 0x00
	};

	murmfd = murmur_connect(cfg.murmur_port);
	if (murmfd < 0)
		return NULL;

	if (sock_write(murmfd, getUsers_packet, sizeof(getUsers_packet)) < 0) {
		fprintf(stderr, "Error: Failed to send getUsers_packet\n");
		close(murmfd);
		return NULL;
	}
	if (sock_read(murmfd, read_buffer, USERLIST_BUFFER_SIZE) < 0) {
		fprintf(stderr, "Error: Failed to receive getUsers_packet reply\n");
		close(murmfd);
		return NULL;
	}
	user_list = malloc_w(READ_BUFFER_SIZE);

	/* read_buffer[25] = number of users */
	bytes_written = snprintf(user_list, READ_BUFFER_SIZE, "%u Online Client%s%s", read_buffer[25],
		(read_buffer[25] == 1 ? "" : "s"), (read_buffer[25] == 0 ? "" : ": "));

	while (user_counter++ < read_buffer[25]) {
		if (user_counter > 1) {
			while (!(username[0] == 0x0 && username[1] == 0x0 && username[2] == 0xff && username[3] == 0xff))
				username++;

			username += 0x2d;
		}
		username[(unsigned) *(username - 1)] = '\0';
		bytes_written += snprintf(user_list + bytes_written, READ_BUFFER_SIZE - bytes_written,
			(user_counter < read_buffer[25] ? "%s, " : "%s"), username);
	}

	close(murmfd);
	return user_list;
}

STATIC ssize_t validate_murmur_connection(int murm_acceptfd) {

	ssize_t n;
	const unsigned char validate_packet[] = {
		0x49, 0x63, 0x65, 0x50, 0x01, 0x00, 0x01, 0x00, 0x03, 0x00, 0x0e, 0x00, 0x00, 0x00
	};

	n = sock_write_non_blocking(murm_acceptfd, validate_packet, sizeof(validate_packet));
	if (n < 0)
		fprintf(stderr, "Error: Failed to send validate_packet\n");

	return n;
}

int accept_murmur_connection(int murm_listenfd) {

	int murm_acceptfd;

	murm_acceptfd = sock_accept(murm_listenfd, NONBLOCK);
	if (murm_acceptfd == -1)
		return -1;

	if (validate_murmur_connection(murm_acceptfd) < 0) {
		close(murm_acceptfd);
		return -1;
	}
	return murm_acceptfd;
}

bool listen_murmur_callbacks(Irc server, int murm_acceptfd) {

	char *username, read_buffer[READ_BUFFER_SIZE];

	errno = 0;
	while (sock_read_non_blocking(murm_acceptfd, read_buffer, sizeof(read_buffer)) > 0) {
		/* Close connection when related packet received */
		if (read_buffer[8] == 0x4)
			break;

		/* Determine if received packet represents userConnected callback */
		if (read_buffer[62] == 'C') {
			username = read_buffer + 99;
			username[(unsigned) *(username - 1)] = '\0';
			send_message(server, default_channel(server), "Mumble: %s connected", username);
		}
	}
	if (errno == EAGAIN)
		return true;

	close(murm_acceptfd);
	return false;
}
