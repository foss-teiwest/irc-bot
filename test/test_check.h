#ifndef CHECK_CHECK_H
#define CHECK_CHECK_H

#include <check.h>

#define READ  0
#define WRITE 1

void socketpair_open(void);
void socketpair_close(void);
void mock_write(int fd[2], const void *buffer, size_t len);
Suite *socket_suite(void);

#endif

