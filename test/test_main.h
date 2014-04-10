#ifndef CHECK_CHECK_H
#define CHECK_CHECK_H

#include <check.h>
#include "socket.h"
#include "irc.h"

struct irc_type {
	int conn;
	int pipe[RDWR];
	int queue;
	char line[IRCLEN + 1];
	size_t line_offset;
	char address[ADDRLEN + 1];
	char port[PORTLEN + 1];
	char nick[NICKLEN + 1];
	char user[USERLEN + 1];
	char channels[MAXCHANS][CHANLEN + 1];
	int channels_set;
	bool connected;
};

extern Irc server;
extern int mock[RDWR];
extern Parsed_data pdata;
extern char test_buffer[IRCLEN + 1];

void connect_irc(void);
void disconnect_irc(void);
void mock_irc_read(void);
void mock_irc_write(void);
void mock_start(void);
void mock_stop(void);

Suite *socket_suite(void);
Suite *irc_suite(void);
Suite *curl_suite(void);
Suite *common_suite(void);

#endif

