#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <stdint.h>
#include <errno.h>
#include "socket.h"
#include "irc.h"
#include "murmur.h"

static int murm_callbackfd;
extern int murm_listenfd, murm_acceptfd;
static uint16_t listener_port = CB_LISTEN_PORT;
static const unsigned char *listener_port_bytes = (unsigned char *)&listener_port;
static void remove_murmur_callbacks(); /* Remove callbacks when listen is about to stop */

int add_murmur_callbacks(const char *port) {

	unsigned char read_buffer[READ_BUFFER_SIZE];

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

	puts("Creating callbacks...");

	if ((murm_callbackfd = sock_connect("127.0.0.1", port)) < 0)
		return 1;

	if (read(murm_callbackfd, read_buffer, READ_BUFFER_SIZE) != VALIDATE_CONNECTION_PACKET_SIZE) {
		fprintf(stderr, "Error: Failed to receive validate_packet. %s\n", strerror(errno));
		return 1;
	}
	if (write(murm_callbackfd, ice_isA_packet, sizeof(ice_isA_packet)) < 0) {
		fprintf(stderr, "Error: Failed to send ice_isA_packet. %s\n", strerror(errno));
		return 1;
	}
	if (read(murm_callbackfd, read_buffer, READ_BUFFER_SIZE) != ICE_ISA_REPLY_PACKET_SIZE) {
		fprintf(stderr, "Error: Failed to receive ice_isA_packet success reply. %s\n", strerror(errno));
		return 1;
	}
	if (write(murm_callbackfd, addCallback_packet, sizeof(addCallback_packet)) < 0) {
		fprintf(stderr, "Error: Failed to send addCallback_packet. %s\n", strerror(errno));
		return 1;
	}
	if (read(murm_callbackfd, read_buffer, READ_BUFFER_SIZE) != ADDCALLBACK_REPLY_PACKET_SIZE) {
		fprintf(stderr, "Error: Failed to receive addCallback_packet success reply. %s\n", strerror(errno));
		return 1;
	}
	signal(SIGINT, remove_murmur_callbacks); /* Handle Ctrl + c */
	signal(SIGTERM, remove_murmur_callbacks);
	atexit(remove_murmur_callbacks);
	return 0;
}

static void remove_murmur_callbacks() {

	unsigned char read_buffer[READ_BUFFER_SIZE];

	const unsigned char removeCallback_packet[] = {
		0x49, 0x63, 0x65, 0x50, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x75, 0x00, 0x00, 0x00, 0x05, 0x00,
		0x00, 0x00, 0x04, 0x4d, 0x65, 0x74, 0x61, 0x00, 0x00, 0x0e, 0x72, 0x65, 0x6d, 0x6f, 0x76, 0x65,
		0x43, 0x61, 0x6c, 0x6c, 0x62, 0x61, 0x63, 0x6b, 0x00, 0x00, 0x4b, 0x00, 0x00, 0x00, 0x01, 0x00,
		0x24, 0x44, 0x35, 0x35, 0x31, 0x30, 0x41, 0x33, 0x35, 0x2d, 0x46, 0x41, 0x38, 0x31, 0x2d, 0x34,
		0x42, 0x38, 0x30, 0x2d, 0x41, 0x43, 0x32, 0x32, 0x2d, 0x41, 0x41, 0x30, 0x35, 0x32, 0x46, 0x42,
		0x34, 0x44, 0x41, 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x19, 0x00, 0x00, 0x00,
		0x01, 0x00, 0x09, 0x31, 0x32, 0x37, 0x2e, 0x30, 0x2e, 0x30, 0x2e, 0x31, listener_port_bytes[1],
		listener_port_bytes[0], 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00
	};
	const unsigned char close_connection_packet[] = {
		0x49, 0x63, 0x65, 0x50, 0x01, 0x00, 0x01, 0x00, 0x04, 0x01, 0x0e, 0x00, 0x00, 0x00
	};

	puts("Removing callbacks...");

	if (write(murm_callbackfd, removeCallback_packet, sizeof(removeCallback_packet)) < 0)
		fprintf(stderr, "Error: Failed to send removeCallback_packet. %s\n", strerror(errno));

	if (read(murm_callbackfd, read_buffer, READ_BUFFER_SIZE) < REMOVECALLBACK_REPLY_PACKET_SIZE)
		fprintf(stderr, "Error: Failed to receive removeCallback_packet success reply. %s\n", strerror(errno));

	if (murm_acceptfd == -1)
		goto shutdown_socks;

	fcntl(murm_acceptfd, F_SETFL, 0); /* Remove non-block mode */
	if (write(murm_acceptfd, close_connection_packet, sizeof(close_connection_packet)) < 0)
		fprintf(stderr, "Error: Failed to send close_connection_packet. %s\n", strerror(errno));

	if(read(murm_acceptfd, read_buffer, READ_BUFFER_SIZE) < 0)
		fprintf(stderr, "Error: Failed to ack close_connection_packet. %s\n", strerror(errno));

	close(murm_acceptfd);

shutdown_socks:
	close(murm_callbackfd);
	if (murm_listenfd != 0)
		close(murm_listenfd);

	exit(EXIT_SUCCESS);
}

ssize_t validate_murmur_connection(int murm_acceptfd) {

	ssize_t n;
	const unsigned char validate_packet[] =	{
		0x49, 0x63, 0x65, 0x50, 0x01, 0x00, 0x01, 0x00, 0x03, 0x00, 0x0e, 0x00, 0x00, 0x00
	};
	if ((n = write(murm_acceptfd, validate_packet, sizeof(validate_packet))) < 0)
		fprintf(stderr, "Error: Failed to send validate_packet. %s\n", strerror(errno));

	return n;
}

int listen_murmur_callbacks(Irc server) {

	ssize_t n;
	char *username, read_buffer[READ_BUFFER_SIZE];

	while ((n = read(murm_acceptfd, read_buffer, sizeof(read_buffer))) > 0) {
		/* Close connection when related packet received */
		if (read_buffer[8] == 0x4)
			return -1;

		/* Determine if received packet represents userConnected or userDisconnected callback */
		if (read_buffer[62] == 'C') { /* Connected */
			username = read_buffer + 99;
			username[(unsigned)read_buffer[98]] = '\0';
			send_message(server, default_channel(server), "Mumble: %s connected", username);
		}
		else if (read_buffer[62] == 'D') { /* Disconnected */
			username = read_buffer + 102;
			username[(unsigned)read_buffer[101]] = '\0';
			send_message(server, default_channel(server), "Mumble: %s disconnected", username);
		}
	}
	return errno == EAGAIN ? 1 : n;
}
