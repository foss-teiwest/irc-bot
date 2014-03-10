#ifndef CHECK_CHECK_H
#define CHECK_CHECK_H

#include <check.h>

#define READ  0
#define WRITE 1

void mock_start(void);
void mock_stop(void);
void mock_write(const void *buffer, size_t len);
Suite *socket_suite(void);
Suite *irc_suite(void);

#endif

