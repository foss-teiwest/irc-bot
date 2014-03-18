#ifndef CHECK_CHECK_H
#define CHECK_CHECK_H

#include <check.h>
#include "irc.h"

#define READ  0
#define WRITE 1

extern Irc server;
extern int mock[2];
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

