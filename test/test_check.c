#include <check.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "test_check.h"

int fd[2];

void socketpair_open(void) {

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd))
		ck_abort_msg("socketpair failed");
}

void socketpair_close(void) {

	close(fd[READ]);
}

void mock_write(int fd[2], const void *buffer, size_t len) {

	switch(fork()) {
	case -1:
		ck_abort_msg("fork failed");
	case 0:
		close(fd[READ]);
		if (write(fd[WRITE], buffer, len) != (int) len)
			ck_abort_msg("write failed");

		close(fd[WRITE]);
		_exit(0);
	}
	close(fd[WRITE]);
}

