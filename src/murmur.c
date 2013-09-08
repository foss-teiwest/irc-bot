#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <errno.h>
#include "socket.h"
#include "irc.h"
#include "murmur.h"


bool add_murmur_callbacks(const char *port) {

	uint16_t listener_port = htons(CB_LISTEN_PORT);
	unsigned char *listener_port_bytes = (unsigned char *)&listener_port;
	unsigned char read_buffer[READ_BUFFER_SIZE];
	int murm_callbackfd;

	const unsigned char ice_isA_packet[] = {
		0x49, 0x63, 0x65, 0x50, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x01, 0x00,
		0x00, 0x00, 0x04, 0x4d, 0x65, 0x74, 0x61, 0x00, 0x00, 0x07, 0x69, 0x63, 0x65, 0x5f, 0x69, 0x73,
		0x41, 0x01, 0x00, 0x15, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0e, 0x3a, 0x3a, 0x4d, 0x75, 0x72, 0x6d,
		0x75, 0x72, 0x3a, 0x3a, 0x4d, 0x65, 0x74, 0x61
	};
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

	if ((murm_callbackfd = sock_connect("127.0.0.1", port)) < 0)
		return false;

	if (read(murm_callbackfd, read_buffer, READ_BUFFER_SIZE) != VALIDATE_CONNECTION_PACKET_SIZE) {
		fprintf(stderr, "Error: Failed to receive validate_packet. %s\n", strerror(errno));
		goto cleanup;
	}
	if (write(murm_callbackfd, ice_isA_packet, sizeof(ice_isA_packet)) < 0) {
		fprintf(stderr, "Error: Failed to send ice_isA_packet. %s\n", strerror(errno));
		goto cleanup;
	}
	if (read(murm_callbackfd, read_buffer, READ_BUFFER_SIZE) != ICE_ISA_REPLY_PACKET_SIZE) {
		fprintf(stderr, "Error: Failed to receive ice_isA_packet success reply. %s\n", strerror(errno));
		goto cleanup;
	}
	if (write(murm_callbackfd, addCallback_packet, sizeof(addCallback_packet)) < 0) {
		fprintf(stderr, "Error: Failed to send addCallback_packet. %s\n", strerror(errno));
		goto cleanup;
	}
	if (read(murm_callbackfd, read_buffer, READ_BUFFER_SIZE) != ADDCALLBACK_REPLY_PACKET_SIZE) {
		fprintf(stderr, "Error: Failed to receive addCallback_packet success reply. %s\n", strerror(errno));
		goto cleanup;
	}
	return true; // Everything succeeded

cleanup:
	close(murm_callbackfd);
	return false;
}

static ssize_t validate_murmur_connection(int murm_acceptfd) {

	ssize_t n;
	const unsigned char validate_packet[] =	{
		0x49, 0x63, 0x65, 0x50, 0x01, 0x00, 0x01, 0x00, 0x03, 0x00, 0x0e, 0x00, 0x00, 0x00
	};
	if ((n = write(murm_acceptfd, validate_packet, sizeof(validate_packet))) < 0)
		fprintf(stderr, "Error: Failed to send validate_packet. %s\n", strerror(errno));

	return n;
}

int accept_murmur_connection(int murm_listenfd) {

	int murm_acceptfd;

	murm_acceptfd = sock_accept(murm_listenfd);
	if (murm_acceptfd > 0 && validate_murmur_connection(murm_acceptfd) > 0) {
		fcntl(murm_acceptfd, F_SETFL, O_NONBLOCK);
		return murm_acceptfd;
	}
	return -1;
}

bool listen_murmur_callbacks(Irc server, int murm_acceptfd) {

	char *username, read_buffer[READ_BUFFER_SIZE];

	while (read(murm_acceptfd, read_buffer, sizeof(read_buffer)) > 0) {
		/* Close connection when related packet received */
		if (read_buffer[8] == 0x4)
			return false;

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
